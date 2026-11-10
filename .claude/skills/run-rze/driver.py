#!/usr/bin/env python3
"""Drives the RZE Editor (or Game) on Linux (KDE Plasma, Wayland), as driver.ps1 does on Windows.

Real input goes through virtual devices (uinput): an absolute pointer, so a click lands on the exact pixel, and a
keyboard. KWin, over D-Bus, finds the window, focuses it, reports where it is and closes it. spectacle takes the
screenshots. Nothing in the engine knows it's being driven.

`launch` starts a helper for the session: it holds the input devices (KWin takes about a second to pick up new
ones) and the Editor's process, so it has the Editor's output and exit code. Each other command talks to it over a
Unix socket and returns at once. Coordinates are client-area pixels, the same space as `shot`'s images.

Usage: driver.py <command> [options]; driver.py <command> -h for each command's options.
"""

import argparse
import bisect
import datetime
import fcntl
import json
import os
import shutil
import signal
import socket
import struct
import subprocess
import sys
import tempfile
import threading
import time

REPO = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
ENGINE_ROOT = os.path.join(REPO, "RZE")
STATE_DIR = os.path.join(os.environ.get("XDG_RUNTIME_DIR") or tempfile.gettempdir(), "rze-driver")
SOCKET_PATH = os.path.join(STATE_DIR, "helper.sock")
APP_LOG = os.path.join(STATE_DIR, "app.log")
HELPER_LOG = os.path.join(STATE_DIR, "helper.log")
# Every Editor run rewrites the layout, which often holds one the user means to keep (CLAUDE.md), so a session puts back
# the file it found. The backup outlives a session that died, and the next launch restores it first.
IMGUI_INI = os.path.join(ENGINE_ROOT, "Config", "imgui.ini")
IMGUI_INI_BACKUP = os.path.join(STATE_DIR, "imgui.ini.backup")
# In place of the backup when there was no layout (it isn't tracked, so a fresh clone has none): putting that back
# removes the one the session wrote, and the next Editor run builds the default layout again
IMGUI_INI_ABSENT = os.path.join(STATE_DIR, "imgui.ini.absent")
# The layout a nested session runs with, in place of the user's, so the coordinates SKILL.md gives for nested sessions
# hold however the user has arranged the panels since. The user's file is backed up and put back as usual.
NESTED_IMGUI_INI = os.path.join(os.path.dirname(os.path.abspath(__file__)), "nested-imgui.ini")
ENGINE_LOGS = os.path.join(ENGINE_ROOT, "Logs")
RENDERDOC_DIR = os.path.join(STATE_DIR, "renderdoc")
SHOT_DIR = os.path.join(tempfile.gettempdir(), "rze-shots")
# Puts the app's windows on one monitor, which Wayland doesn't let the app do itself (launch --output)
LAUNCH_ON_OUTPUT = os.path.join(REPO, "Tools", "Linux", "LaunchOnOutput.py")
LAUNCH_ON_OUTPUT_LOG = os.path.join(STATE_DIR, "launch-on-output.log")
# A nested session's KWin (launch --nested): its Wayland socket, its log, and its process group, kept on disk so the
# next launch can end one a dead session left
NESTED_SOCKET = "rze-driver-nested"
NESTED_LOG = os.path.join(STATE_DIR, "nested.log")
NESTED_GROUP_FILE = os.path.join(STATE_DIR, "nested.pgid")
NESTED_SIZE = (2560, 1440)
NESTED_START_TIMEOUT_S = 15
# The session's own config, data, state and cache folders (XDG_*_HOME), made fresh for each session. KWin and the
# services its bus starts write there, not to the desktop's: with the desktop's, a nested KWin records its virtual
# output in ~/.config/kwinoutputconfig.json, beside the real monitors' layouts.
NESTED_HOME = os.path.join(STATE_DIR, "nested-home")
NESTED_XDG_DIRS = {"XDG_CONFIG_HOME": "config", "XDG_DATA_HOME": "data", "XDG_STATE_HOME": "state",
                   "XDG_CACHE_HOME": "cache"}
# The virtual output's refresh rate: KWin's virtual backend starts at 60 Hz, and the desktop's monitors here run at 240
NESTED_REFRESH_HZ = 240


class DriverError(Exception):
    pass


# ---------------------------------------------------------------------------------------------------------------------
# KWin: scripts loaded over D-Bus. A script's print() is filtered out of KWin's log, so each one sends its answer back
# with callDBus to a name this process owns.

KWIN_REPLY_NAME = "org.rze.Driver"
KWIN_REPLY_XML = ("<node><interface name='org.rze.Driver'><method name='Report'>"
                  "<arg type='s' name='json' direction='in'/></method></interface></node>")

# Each window as the driver needs it. internalId stays the same for the window's life.
KWIN_WINDOW_JS = """
function describe(w) {
    return { id: w.internalId.toString(), pid: w.pid, caption: w.caption, normal: w.normalWindow, utility: w.utility,
             client: [w.clientGeometry.x, w.clientGeometry.y, w.clientGeometry.width, w.clientGeometry.height],
             active: workspace.activeWindow == w, minimized: w.minimized };
}
function byId(id) {
    var found = null;
    workspace.windowList().forEach(function (w) { if (w.internalId.toString() == id) found = w; });
    return found;
}
"""


# The D-Bus the driver's KWin scripts go to: None for the desktop's session bus, or a nested session's address
kwin_bus_address = None


def kwin_bus():
    from gi.repository import Gio

    if kwin_bus_address is None:
        return Gio.bus_get_sync(Gio.BusType.SESSION)
    flags = Gio.DBusConnectionFlags.AUTHENTICATION_CLIENT | Gio.DBusConnectionFlags.MESSAGE_BUS_CONNECTION
    return Gio.DBusConnection.new_for_address_sync(kwin_bus_address, flags, None, None)


def kwin(body, timeout_ms=3000):
    """Runs body as a KWin script; it calls report(value) once, and that value is returned."""
    from gi.repository import Gio, GLib

    bus = kwin_bus()
    result = {}
    loop = GLib.MainLoop()

    def on_call(_conn, _sender, _path, _iface, _method, params, invocation):
        result["json"] = params.unpack()[0]
        invocation.return_value(None)
        loop.quit()

    node = Gio.DBusNodeInfo.new_for_xml(KWIN_REPLY_XML)
    registration = bus.register_object_with_closures2("/", node.interfaces[0], on_call, None, None)
    owner = Gio.bus_own_name_on_connection(bus, KWIN_REPLY_NAME, Gio.BusNameOwnerFlags.NONE, None, None)
    script = ("function report(v) { callDBus('%s', '/', 'org.rze.Driver', 'Report', JSON.stringify(v)); }\n%s\n%s\n"
              % (KWIN_REPLY_NAME, KWIN_WINDOW_JS, body))
    fd, path = tempfile.mkstemp(prefix="rze-kwin-", suffix=".js")
    os.write(fd, script.encode())
    os.close(fd)
    name = "rzedriver%d_%d" % (os.getpid(), int(time.monotonic() * 1000))

    def call(obj, iface, method, args):
        return bus.call_sync("org.kde.KWin", obj, iface, method, args, None, Gio.DBusCallFlags.NONE, -1, None)

    try:
        script_id = call("/Scripting", "org.kde.kwin.Scripting", "loadScript", GLib.Variant("(ss)", (path, name))).unpack()[0]
        call("/Scripting/Script%d" % script_id, "org.kde.kwin.Script", "run", None)
        GLib.timeout_add(timeout_ms, loop.quit)
        loop.run()
    finally:
        try:
            call("/Scripting", "org.kde.kwin.Scripting", "unloadScript", GLib.Variant("(s)", (name,)))
        finally:
            Gio.bus_unown_name(owner)
            bus.unregister_object(registration)
            os.unlink(path)
            if kwin_bus_address is not None:
                bus.close_sync(None)
    if "json" not in result:
        raise DriverError("KWin didn't answer (is this a KDE Plasma session?)")
    return json.loads(result["json"])


def kwin_windows():
    return kwin("var out = []; workspace.windowList().forEach(function (w) { out.push(describe(w)); }); report(out);")


def kwin_outputs():
    """The monitors, as KWin names them, each with its x, y, width, height on the desktop."""
    return kwin("""report(workspace.screens.map(function (s) {
    return { name: s.name, geometry: [s.geometry.x, s.geometry.y, s.geometry.width, s.geometry.height] };
}));""")


def describe_outputs():
    return "\n".join("%s\t%d,%d %dx%d" % ((o["name"],) + tuple(o["geometry"])) for o in kwin_outputs())


def kwin_desktop_size():
    width, height = kwin("""var w = 0, h = 0;
workspace.screens.forEach(function (s) { w = Math.max(w, s.geometry.x + s.geometry.width); h = Math.max(h, s.geometry.y + s.geometry.height); });
report([w, h]);""")
    return int(width), int(height)


# ---------------------------------------------------------------------------------------------------------------------
# uinput: virtual input devices, with no modules beyond the standard library

EV_SYN, EV_KEY, EV_REL, EV_ABS = 0x00, 0x01, 0x02, 0x03
SYN_REPORT = 0
REL_WHEEL = 0x08
ABS_X, ABS_Y = 0x00, 0x01
BUTTONS = {"left": 0x110, "right": 0x111, "middle": 0x112}
INPUT_PROP_POINTER = 0x00
BUS_VIRTUAL = 0x06


def _iow(nr, size):
    return (1 << 30) | (size << 16) | (ord("U") << 8) | nr


UI_SET_EVBIT, UI_SET_KEYBIT, UI_SET_RELBIT, UI_SET_ABSBIT, UI_SET_PROPBIT = (
    _iow(100, 4), _iow(101, 4), _iow(102, 4), _iow(103, 4), _iow(110, 4))
UI_DEV_SETUP = _iow(3, 92)    # struct uinput_setup: input_id, char name[80], u32 ff_effects_max
UI_ABS_SETUP = _iow(4, 28)    # struct uinput_abs_setup: u16 code, struct input_absinfo
UI_DEV_CREATE = (ord("U") << 8) | 1
UI_DEV_DESTROY = (ord("U") << 8) | 2

# Linux key codes (input-event-codes.h), by SendKeys-style name
KEYS = {
    "ESC": 1, "BACKSPACE": 14, "BS": 14, "TAB": 15, "ENTER": 28, "SPACE": 57, "CAPSLOCK": 58,
    "HOME": 102, "UP": 103, "PGUP": 104, "LEFT": 105, "RIGHT": 106, "END": 107, "DOWN": 108, "PGDN": 109,
    "INSERT": 110, "INS": 110, "DELETE": 111, "DEL": 111, "PRTSC": 99, "PAUSE": 119,
    "F1": 59, "F2": 60, "F3": 61, "F4": 62, "F5": 63, "F6": 64, "F7": 65, "F8": 66, "F9": 67, "F10": 68,
    "F11": 87, "F12": 88,
}
KEY_LEFTCTRL, KEY_LEFTSHIFT, KEY_LEFTALT = 29, 42, 56
MODIFIERS = {"^": KEY_LEFTCTRL, "+": KEY_LEFTSHIFT, "%": KEY_LEFTALT}

# A US layout's characters: (key code, needs Shift)
CHARS = {}
for _row, _first in (("1234567890", 2), ("qwertyuiop", 16), ("asdfghjkl", 30), ("zxcvbnm", 44)):
    for _i, _c in enumerate(_row):
        CHARS[_c] = (_first + _i, False)
        if _c.isalpha():
            CHARS[_c.upper()] = (_first + _i, True)
for _c, _code in (("-", 12), ("=", 13), ("[", 26), ("]", 27), (";", 39), ("'", 40), ("`", 41), ("\\", 43), (",", 51),
                  (".", 52), ("/", 53), (" ", 57), ("\t", 15), ("\n", 28)):
    CHARS[_c] = (_code, False)
for _c, _base in zip('!@#$%^&*()_+{}:"~|<>?', '1234567890-=[];\'`\\,./'):
    CHARS[_c] = (CHARS[_base][0], True)


class VirtualDevice:
    def __init__(self, name, keys=(), abs_size=None, wheel=False):
        self.fd = os.open("/dev/uinput", os.O_WRONLY | os.O_NONBLOCK)
        fcntl.ioctl(self.fd, UI_SET_EVBIT, EV_SYN)
        fcntl.ioctl(self.fd, UI_SET_EVBIT, EV_KEY)
        for key in keys:
            fcntl.ioctl(self.fd, UI_SET_KEYBIT, key)
        if wheel:
            fcntl.ioctl(self.fd, UI_SET_EVBIT, EV_REL)
            fcntl.ioctl(self.fd, UI_SET_RELBIT, REL_WHEEL)
        if abs_size:
            fcntl.ioctl(self.fd, UI_SET_EVBIT, EV_ABS)
            for axis, size in zip((ABS_X, ABS_Y), abs_size):
                fcntl.ioctl(self.fd, UI_SET_ABSBIT, axis)
                # 0..size, not size - 1: libinput scales by size / max, so this maps each value to its own pixel
                fcntl.ioctl(self.fd, UI_ABS_SETUP, struct.pack("<HHiiiiii", axis, 0, 0, 0, size, 0, 0, 0))
            fcntl.ioctl(self.fd, UI_SET_PROPBIT, INPUT_PROP_POINTER)
        fcntl.ioctl(self.fd, UI_DEV_SETUP, struct.pack("<HHHH80sI", BUS_VIRTUAL, 0x525a, 0x4501, 1, name.encode(), 0))
        fcntl.ioctl(self.fd, UI_DEV_CREATE)

    def emit(self, events):
        data = b"".join(struct.pack("<qqHHi", 0, 0, t, c, v) for t, c, v in events)
        os.write(self.fd, data + struct.pack("<qqHHi", 0, 0, EV_SYN, SYN_REPORT, 0))

    def close(self):
        fcntl.ioctl(self.fd, UI_DEV_DESTROY)
        os.close(self.fd)


def parse_keys(text):
    """SendKeys syntax, as driver.ps1 takes: ^ Ctrl, + Shift, % Alt (on the next key, or a (group)), {NAME} or
    {NAME n} for named keys, {+} {^} {%} {~} {(} {)} {{} {}} for those characters, ~ for Enter. Returns a list of
    (modifier codes, key code, needs Shift)."""
    strokes, i, held, group = [], 0, [], None
    while i < len(text):
        c = text[i]
        if c in MODIFIERS:
            held.append(MODIFIERS[c])
            i += 1
            continue
        if c == "(" and held:
            group, held = held, []
            i += 1
            continue
        if c == ")" and group is not None:
            group = None
            i += 1
            continue
        if c == "{":
            end = text.find("}", i + 2 if text.startswith("{}", i) else i + 1)
            if end < 0:
                raise DriverError("Unclosed { in keys: %r" % text)
            body = text[i + 1:end]
            name, _, count = body.partition(" ")
            count = int(count) if count else 1
            if name.upper() in KEYS:
                code, shift = KEYS[name.upper()], False
            elif len(name) == 1 and name in CHARS:
                code, shift = CHARS[name]
            else:
                raise DriverError("Unknown key {%s}" % body)
            i = end + 1
        else:
            if c == "~":
                code, shift = KEYS["ENTER"], False
            elif c in CHARS:
                code, shift = CHARS[c]
            else:
                raise DriverError("Can't type %r on a US layout" % c)
            count = 1
            i += 1
        modifiers = list(group or []) + held
        strokes += [(modifiers, code, shift)] * count
        held = []
    return strokes


# ---------------------------------------------------------------------------------------------------------------------
# Nested sessions (launch --nested): a KWin of the session's own, drawing to an off-screen output, on a D-Bus session of
# its own. The app's windows, and anything it opens (a Game, a file manager), live there, so nothing reaches the desktop:
# no window appears, focus and the clipboard are left alone. Harness sessions only, as nothing gives it input devices.

def stop_leftover_nested():
    """Ends a nested session a dead helper left running. Says whether there was one."""
    try:
        with open(NESTED_GROUP_FILE) as f:
            group = int(f.read())
    except (OSError, ValueError):
        return False
    os.unlink(NESTED_GROUP_FILE)
    # Only if the group's leader is still the bus daemon this driver started, not a process that reused its PID
    try:
        with open("/proc/%d/cmdline" % group, "rb") as f:
            if not f.read().startswith(b"dbus-daemon\0"):
                return False
    except OSError:
        return False
    stop_group(group)
    return True


def stop_group(group, timeout=5):
    """SIGTERM, then SIGKILL once timeout passes, for every process in the group; waits until they're gone."""
    for sig in (signal.SIGTERM, signal.SIGKILL):
        try:
            os.killpg(group, sig)
        except ProcessLookupError:
            return
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            try:
                os.killpg(group, 0)
            except ProcessLookupError:
                return
            time.sleep(0.05)


class NestedSession:
    def __init__(self, refresh_hz):
        log = open(NESTED_LOG, "wb")
        # The desktop's, so nothing in the session finds it: KWin would otherwise talk to the desktop's compositor
        environment = {k: v for k, v in os.environ.items()
                       if k not in ("WAYLAND_DISPLAY", "DISPLAY", "XAUTHORITY", "DBUS_SESSION_BUS_ADDRESS")}
        shutil.rmtree(NESTED_HOME, ignore_errors=True)
        for name, folder in NESTED_XDG_DIRS.items():
            environment[name] = os.path.join(NESTED_HOME, folder)
            os.makedirs(environment[name], mode=0o700)
        self.environment = environment
        # The group is the bus daemon's, and KWin joins it: ending the group ends the session, with the services the
        # bus and KWin start (portals, plasma-keyboard, ksecretd). A group, not a session: a process can only join a
        # group in its own session, and the helper already has a session of its own.
        self.bus = subprocess.Popen(["dbus-daemon", "--session", "--nofork", "--print-address=1"], env=environment,
                                    stdout=subprocess.PIPE, stderr=log, stdin=subprocess.DEVNULL, process_group=0)
        self.group = self.bus.pid
        with open(NESTED_GROUP_FILE, "w") as f:
            f.write(str(self.group))
        try:
            self.address = self.bus.stdout.readline().decode().strip()
            if not self.address:
                raise DriverError("The nested session's D-Bus didn't start; see %s" % NESTED_LOG)
            environment["DBUS_SESSION_BUS_ADDRESS"] = self.address
            width, height = NESTED_SIZE
            self.kwin = subprocess.Popen(["kwin_wayland", "--virtual", "--width", str(width), "--height", str(height),
                                          "--socket", NESTED_SOCKET, "--no-lockscreen", "--no-global-shortcuts"],
                                         env=environment, stdout=log, stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL,
                                         process_group=self.group)
            self._wait_for_kwin()
            self.refresh_hz = self._set_refresh(refresh_hz)
        except BaseException:
            self.stop()
            raise

    def _wait_for_kwin(self):
        from gi.repository import Gio, GLib

        flags = Gio.DBusConnectionFlags.AUTHENTICATION_CLIENT | Gio.DBusConnectionFlags.MESSAGE_BUS_CONNECTION
        bus = Gio.DBusConnection.new_for_address_sync(self.address, flags, None, None)
        socket_path = os.path.join(os.environ["XDG_RUNTIME_DIR"], NESTED_SOCKET)
        deadline = time.monotonic() + NESTED_START_TIMEOUT_S
        try:
            while True:
                if self.kwin.poll() is not None:
                    raise DriverError("The nested KWin exited (code %d); see %s" % (self.kwin.returncode, NESTED_LOG))
                has_owner = bus.call_sync("org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus",
                                          "NameHasOwner", GLib.Variant("(s)", ("org.kde.KWin",)), None,
                                          Gio.DBusCallFlags.NONE, -1, None).unpack()[0]
                if has_owner and os.path.exists(socket_path):
                    # Services the bus starts (the portals a Qt client asks for) get its environment, which has no
                    # display: they'd fall back to wayland-0 and abort. Plasma's startup does the same for the desktop.
                    bus.call_sync("org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus",
                                  "UpdateActivationEnvironment", GLib.Variant("(a{ss})", ({"WAYLAND_DISPLAY": NESTED_SOCKET},)),
                                  None, Gio.DBusCallFlags.NONE, -1, None)
                    return
                if time.monotonic() > deadline:
                    raise DriverError("The nested KWin didn't start in %ds; see %s" % (NESTED_START_TIMEOUT_S, NESTED_LOG))
                time.sleep(0.1)
        finally:
            bus.close_sync(None)

    def _kscreen_doctor(self, *arguments):
        environment = dict(self.environment, WAYLAND_DISPLAY=NESTED_SOCKET)
        result = subprocess.run(["kscreen-doctor", *arguments], env=environment, capture_output=True, text=True,
                                timeout=20)
        if result.returncode != 0:
            raise DriverError("kscreen-doctor %s failed: %s" % (" ".join(arguments), (result.stderr or result.stdout).strip()))
        return result.stdout

    def _output(self):
        outputs = json.loads(self._kscreen_doctor("-j"))["outputs"]
        if len(outputs) != 1:
            raise DriverError("The nested KWin has %d outputs, not 1" % len(outputs))
        return outputs[0]

    def _set_refresh(self, refresh_hz):
        """Gives the virtual output a mode at refresh_hz (KWin's custom modes) and switches to it; returns the rate it
        has then, as KWin rounds it to a CVT timing (240 is 239.9x)."""
        output = self._output()
        width, height = NESTED_SIZE
        self._kscreen_doctor("output.%s.addCustomMode.%d.%d.%d.full" % (output["name"], width, height, refresh_hz * 1000))
        modes = [m for m in self._output()["modes"] if m["size"] == {"width": width, "height": height}]
        mode = min(modes, key=lambda m: abs(m["refreshRate"] - refresh_hz))
        self._kscreen_doctor("output.%s.mode.%s" % (output["name"], mode["id"]))
        current = next(m for m in self._output()["modes"] if m["id"] == self._output()["currentModeId"])
        if abs(current["refreshRate"] - refresh_hz) > 1:
            raise DriverError("The nested output is at %.2f Hz, not %d" % (current["refreshRate"], refresh_hz))
        return current["refreshRate"]

    def apply(self, environment):
        """Points an app's environment at this session instead of the desktop."""
        environment["WAYLAND_DISPLAY"] = NESTED_SOCKET
        environment["DBUS_SESSION_BUS_ADDRESS"] = self.address
        # Only Wayland: with no X server here, SDL falling back to X11 would open its windows on the desktop's
        environment["SDL_VIDEO_DRIVER"] = "wayland"
        for name in ("DISPLAY", "XAUTHORITY", "XDG_ACTIVATION_TOKEN", "DESKTOP_STARTUP_ID"):
            environment.pop(name, None)

    def stop(self):
        # KWin and the bus are this process's children, so they're reaped here: until then they'd still count as the
        # group's, and stop_group would wait them out. What's left after (services the bus started) isn't.
        children = [p for p in (getattr(self, "kwin", None), self.bus) if p is not None]
        for sig in (signal.SIGTERM, signal.SIGKILL):
            try:
                os.killpg(self.group, sig)
            except ProcessLookupError:
                break
            deadline = time.monotonic() + 5
            for child in children:
                try:
                    child.wait(timeout=max(0.0, deadline - time.monotonic()))
                except subprocess.TimeoutExpired:
                    pass
            if all(child.poll() is not None for child in children):
                break
        stop_group(self.group)
        shutil.rmtree(NESTED_HOME, ignore_errors=True)
        try:
            os.unlink(NESTED_GROUP_FILE)
        except FileNotFoundError:
            pass


# ---------------------------------------------------------------------------------------------------------------------
# The helper: one per launched session

def restore_imgui_ini():
    """Puts back the layout a session found, or that it found none, if it recorded either. A copy, so the file gets a new
    timestamp."""
    if os.path.exists(IMGUI_INI_BACKUP):
        shutil.copyfile(IMGUI_INI_BACKUP, IMGUI_INI)
        os.remove(IMGUI_INI_BACKUP)
        return True
    if os.path.exists(IMGUI_INI_ABSENT):
        if os.path.exists(IMGUI_INI):
            os.remove(IMGUI_INI)
        os.remove(IMGUI_INI_ABSENT)
        return True
    return False


def back_up_imgui_ini():
    if os.path.exists(IMGUI_INI):
        shutil.copyfile(IMGUI_INI, IMGUI_INI_BACKUP)
    else:
        open(IMGUI_INI_ABSENT, "w").close()


class Helper:
    def __init__(self, options):
        self.options = options
        # A backup a dead session left holds the layout from before it, so it goes back first, even when this run keeps
        # its layout: left on disk, this session's close would restore it over the kept one. As driver.ps1 does.
        self.restored_leftover_ini = restore_imgui_ini()
        if not options.get("keep_imgui_ini"):
            back_up_imgui_ini()
            # Not with --keep-imgui-ini: that run is meant to change the user's layout, so it starts from it
            if options.get("nested"):
                shutil.copyfile(NESTED_IMGUI_INI, IMGUI_INI)
        self.lock = threading.Lock()
        self.exit_code = None
        self.window_id = None
        # As the layout backup is: a dead session's nested KWin goes whatever this session is
        self.stopped_leftover_nested = stop_leftover_nested()
        # First, as every KWin script from here on goes to its KWin
        self.nested = NestedSession(options.get("refresh") or NESTED_REFRESH_HZ) if options.get("nested") else None
        try:
            self._start_session(options)
        except BaseException:
            if self.nested is not None:
                self.nested.stop()
            raise

    def _start_session(self, options):
        global kwin_bus_address
        if self.nested is not None:
            kwin_bus_address = self.nested.address
        self.windows_at_launch = {w["id"] for w in kwin_windows()}
        self.pointer = None
        self.keyboard = None
        if not options.get("harness"):
            self.pointer = VirtualDevice("RZE driver pointer", keys=BUTTONS.values(), abs_size=kwin_desktop_size(), wheel=True)
            self.keyboard = VirtualDevice("RZE driver keyboard", keys=list(range(1, 128)))
        self.devices_ready_at = time.monotonic() + 0.8

        build_dir = os.path.join(ENGINE_ROOT, "_build", options["config"])
        exe = os.path.join(build_dir, options["app"])
        # Line-buffered, so `log` sees each line as it's written
        command = ["stdbuf", "-oL", "-eL"]
        environment = dict(os.environ)
        if options.get("renderdoc"):
            # RenderDoc turns capturing off for the whole process (its overlay says "captures disabled") once anything
            # calls GL it doesn't support. The splash's window surface would: SDL draws window surfaces with its
            # OpenGL renderer when it can, through fixed-function calls (glMatrixMode, glOrtho, ...). So the app runs on
            # XWayland, where SDL has a native framebuffer for them, and RenderDoc's capture key (read through xlib,
            # since this build has no Wayland support) works too.
            shutil.rmtree(RENDERDOC_DIR, ignore_errors=True)
            os.makedirs(RENDERDOC_DIR)
            environment["SDL_VIDEO_DRIVER"] = "x11"
            environment["SDL_FRAMEBUFFER_ACCELERATION"] = "0"
            command += ["renderdoccmd", "capture", "--wait-for-exit", "--working-dir", build_dir,
                        "--capture-file", os.path.join(RENDERDOC_DIR, options["app"])]
        if options.get("output"):
            # Right before the app: it runs the app in its own process, whose PID its KWin script matches. Before
            # renderdoccmd, it would match renderdoccmd's.
            command += [sys.executable, "-I", LAUNCH_ON_OUTPUT, "--output", options["output"], "--log", LAUNCH_ON_OUTPUT_LOG,
                        "--"]
        if self.nested is not None:
            self.nested.apply(environment)
        command += [exe] + options["args"]
        # In a process group of its own, which is everything the session started: the app, renderdoccmd with the
        # app under it, a Game the Editor launched. Ending the session ends all of it.
        self.process = self._start(command, build_dir, environment)
        threading.Thread(target=self._wait_for_exit, daemon=True).start()

    def _start(self, command, build_dir, environment):
        return subprocess.Popen(command, cwd=build_dir, env=environment, stdout=open(APP_LOG, "wb"),
                                stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL, start_new_session=True)

    def _wait_for_exit(self):
        self.exit_code = self.process.wait()

    def _group_alive(self):
        try:
            os.killpg(self.process.pid, 0)
            return True
        except ProcessLookupError:
            return False

    def _end_group(self, timeout=10):
        """Kills whatever the session started that's still running, and waits until it's gone."""
        if self._group_alive():
            os.killpg(self.process.pid, signal.SIGKILL)
        deadline = time.monotonic() + timeout
        while self._group_alive() and time.monotonic() < deadline:
            time.sleep(0.05)
        if self.exit_code is None:
            self.process.wait()

    # The app's windows: the main one is its largest normal window (the splash is smaller, and goes)
    def app_windows(self):
        pids = {self.process.pid} | self._child_pids()
        return [w for w in kwin_windows() if w["pid"] in pids]

    def _child_pids(self):
        try:
            out = subprocess.run(["pgrep", "-P", str(self.process.pid)], capture_output=True, text=True).stdout
            return {int(p) for p in out.split()}
        except OSError:
            return set()

    def main_window(self):
        windows = [w for w in self.app_windows() if w["normal"] and not w["utility"]]
        if not windows:
            raise DriverError("The app has no window (yet?)")
        return max(windows, key=lambda w: w["client"][2] * w["client"][3])

    def focus(self):
        window = self.main_window()
        if not window["active"]:
            kwin("var w = byId('%s'); if (w) workspace.activeWindow = w; report(1);" % window["id"])
            time.sleep(0.15)
            window = self.main_window()
            if not window["active"]:
                raise DriverError("Couldn't focus the app's window")
        return window

    def _point(self, window, x, y):
        cx, cy, cw, ch = window["client"]
        if not (0 <= x < cw and 0 <= y < ch):
            raise DriverError("(%d, %d) is outside the window's %dx%d client area" % (x, y, cw, ch))
        return int(cx + x), int(cy + y)

    def _ready(self):
        wait = self.devices_ready_at - time.monotonic()
        if wait > 0:
            time.sleep(wait)

    def move_to(self, window, x, y):
        sx, sy = self._point(window, x, y)
        self.pointer.emit([(EV_ABS, ABS_X, sx), (EV_ABS, ABS_Y, sy)])
        time.sleep(0.03)

    def button(self, name, down):
        self.pointer.emit([(EV_KEY, BUTTONS[name], 1 if down else 0)])
        time.sleep(0.03)

    # Commands, each checked against the window just before its input, so input never lands outside the app
    def cmd_click(self, x, y, button="left", count=1):
        self._ready()
        window = self.focus()
        self.move_to(window, x, y)
        for _ in range(count):
            self.button(button, True)
            self.button(button, False)
        time.sleep(0.1)
        return "clicked (%d, %d)" % (x, y)

    def cmd_move(self, x, y):
        self._ready()
        self.move_to(self.focus(), x, y)
        return "moved to (%d, %d)" % (x, y)

    def cmd_drag(self, x, y, x2, y2, button="left", steps=10):
        self._ready()
        window = self.focus()
        self._point(window, x2, y2)
        self.move_to(window, x, y)
        self.button(button, True)
        for step in range(1, steps + 1):
            self.move_to(window, x + (x2 - x) * step / steps, y + (y2 - y) * step / steps)
        self.button(button, False)
        time.sleep(0.1)
        return "dragged (%d, %d) -> (%d, %d)" % (x, y, x2, y2)

    def cmd_scroll(self, x, y, notches):
        self._ready()
        window = self.focus()
        self.move_to(window, x, y)
        for _ in range(abs(notches)):
            self.pointer.emit([(EV_REL, REL_WHEEL, 1 if notches > 0 else -1)])
            time.sleep(0.03)
        return "scrolled %d at (%d, %d)" % (notches, x, y)

    def cmd_key(self, keys):
        strokes = parse_keys(keys)
        self._ready()
        self.focus()
        for modifiers, code, shift in strokes:
            down = modifiers + ([KEY_LEFTSHIFT] if shift and KEY_LEFTSHIFT not in modifiers else [])
            # Held as a hand would hold them, a few milliseconds each, so the app sees every key with its modifiers
            for m in down:
                self.keyboard.emit([(EV_KEY, m, 1)])
                time.sleep(0.01)
            self.keyboard.emit([(EV_KEY, code, 1)])
            time.sleep(0.02)
            self.keyboard.emit([(EV_KEY, code, 0)])
            for m in reversed(down):
                time.sleep(0.01)
                self.keyboard.emit([(EV_KEY, m, 0)])
            time.sleep(0.02)
        return "typed %d keys" % len(strokes)

    def cmd_shot(self, out):
        window = self.focus()
        os.makedirs(os.path.dirname(out), exist_ok=True)
        subprocess.run(["spectacle", "-b", "-n", "-a", "-e", "-o", out], check=True,
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        width, height = png_size(out)
        note = "" if [width, height] == window["client"][2:] else " (the window's client area is %dx%d)" % tuple(window["client"][2:])
        return "%s %dx%d%s" % (out, width, height, note)

    def cmd_info(self):
        window = self.main_window()
        return "%s client=%dx%d at (%d, %d) active=%s" % (window["caption"], window["client"][2], window["client"][3],
                                                         window["client"][0], window["client"][1], window["active"])

    def cmd_windows(self, all_windows=False):
        windows = kwin_windows()
        if not all_windows:
            windows = [w for w in windows if w["id"] not in self.windows_at_launch]
        return "\n".join("pid=%d %s %dx%d at (%d, %d)%s" % (w["pid"], json.dumps(w["caption"]), w["client"][2], w["client"][3],
                                                           w["client"][0], w["client"][1], " active" if w["active"] else "")
                         for w in windows if w["normal"]) or "(none)"

    def cmd_close_window(self, caption):
        windows = [w for w in kwin_windows() if w["id"] not in self.windows_at_launch and w["caption"] == caption]
        if not windows:
            raise DriverError("No window called %r opened since launch" % caption)
        kwin("var w = byId('%s'); if (w) w.closeWindow(); report(1);" % windows[0]["id"])
        return "closed %r" % caption

    def cmd_renderdoc_capture(self, timeout):
        if not self.options.get("renderdoc"):
            raise DriverError("This session isn't under RenderDoc. Run: driver.py launch --renderdoc")
        before = set(renderdoc_captures())
        # RenderDoc's capture key: it captures the next frame the app presents
        self.cmd_key("{F12}")
        deadline = time.monotonic() + timeout
        while True:
            new = [c for c in renderdoc_captures() if c not in before]
            if new and file_settled(new[0]):
                return new[0]
            if time.monotonic() > deadline:
                raise DriverError("No capture after %ds; see the newest /tmp/RenderDoc/*.log" % timeout)
            time.sleep(0.25)

    # The harness's own (HarnessHelper)
    def cmd_state(self):
        raise DriverError("state needs a harness session: driver.py launch --harness")

    def cmd_object(self, name):
        raise DriverError("object needs a harness session: driver.py launch --harness")

    def cmd_realinput(self, on):
        raise DriverError("realinput needs a harness session: driver.py launch --harness")

    def cmd_wait_settled(self):
        return "settled"

    def cmd_restored_leftover_ini(self):
        return str(self.restored_leftover_ini)

    def cmd_alive(self):
        if self.exit_code is None:
            return "running pid=%d" % self.process.pid
        return "exited code=%d" % self.exit_code

    def cmd_close(self, timeout):
        if self.exit_code is None:
            window = self.main_window()
            kwin("var w = byId('%s'); if (w) w.closeWindow(); report(1);" % window["id"])
            deadline = time.monotonic() + timeout
            while self.exit_code is None and time.monotonic() < deadline:
                time.sleep(0.1)
            if self.exit_code is None:
                raise DriverError("Still running after %ds (an unsaved-changes prompt? see `shot`), then `quit`" % timeout)
        # Anything it left running (a Game it launched) goes too, before the layout is put back
        self._end_group()
        return "closed code=%d%s" % (self.exit_code, self._restore_note())

    def cmd_quit(self):
        self._end_group()
        return "quit" + self._restore_note()

    # Once the app has exited, since ImGui saves the layout as it shuts down
    def _restore_note(self):
        return "; imgui.ini restored" if restore_imgui_ini() else ""

    def cmd_wayland_display(self):
        """The Wayland display the app is on, for what the client runs itself (wl-copy): "" for the desktop's."""
        return NESTED_SOCKET if self.nested is not None else ""

    def cmd_nested_mode(self):
        return "%dx%d@%.2f" % (NESTED_SIZE + (self.nested.refresh_hz,))

    def cmd_stopped_leftover_nested(self):
        return str(self.stopped_leftover_nested)

    def shutdown(self):
        self._end_group()
        restore_imgui_ini()
        for device in (self.pointer, self.keyboard):
            if device is not None:
                device.close()
        # Last: the app (and a Game it launched) has gone, with nothing left to draw
        if self.nested is not None:
            self.nested.stop()


class HarnessHelper(Helper):
    """A session with EditorHarness (RZE/EditorHarness): input and screenshots go through the app itself, as JSON lines
    on its stdin, so the real cursor and focus are left alone and the window may be covered. Its answers come back on
    stdout as "RZEHARNESS {json}" lines, among its log lines."""

    REPLY_PREFIX = "RZEHARNESS "

    def __init__(self, options):
        self.replies = {}
        self.replies_changed = threading.Condition()
        self.is_ready = False
        self.next_id = 0
        Helper.__init__(self, options)

    def _start(self, command, build_dir, environment):
        process = subprocess.Popen(command, cwd=build_dir, env=environment, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                   stderr=subprocess.STDOUT, start_new_session=True)
        threading.Thread(target=self._read_output, args=(process.stdout,), daemon=True).start()
        return process

    def _read_output(self, stdout):
        with open(APP_LOG, "wb") as log:
            for raw in stdout:
                line = raw.decode(errors="replace")
                if not line.startswith(self.REPLY_PREFIX):
                    log.write(raw)
                    log.flush()
                    continue
                reply = json.loads(line[len(self.REPLY_PREFIX):])
                with self.replies_changed:
                    if reply.get("event") == "ready":
                        self.is_ready = True
                    elif "id" in reply:
                        self.replies[reply["id"]] = reply
                    self.replies_changed.notify_all()
        with self.replies_changed:
            self.replies_changed.notify_all()

    def request(self, command_name, timeout=30, **fields):
        """Sends one command and waits for its answer; DriverError if the harness says it failed."""
        if self.exit_code is not None:
            raise DriverError("The app has exited (code %d)" % self.exit_code)
        with self.replies_changed:
            self.next_id += 1
            request_id = self.next_id
        self.process.stdin.write((json.dumps(dict(fields, cmd=command_name, id=request_id)) + "\n").encode())
        self.process.stdin.flush()
        deadline = time.monotonic() + timeout
        with self.replies_changed:
            while request_id not in self.replies:
                if self.exit_code is not None or time.monotonic() > deadline:
                    raise DriverError("No answer to %s from the harness%s" % (
                        command_name, " (it exited)" if self.exit_code is not None else " after %ds" % timeout))
                self.replies_changed.wait(0.1)
            reply = self.replies.pop(request_id)
        if not reply.get("ok"):
            raise DriverError(reply.get("error", "failed"))
        return reply

    def _wait_ready(self, timeout=60):
        deadline = time.monotonic() + timeout
        with self.replies_changed:
            while not self.is_ready:
                if self.exit_code is not None or time.monotonic() > deadline:
                    raise DriverError("The harness didn't get ready")
                self.replies_changed.wait(0.1)

    def cmd_wait_settled(self, timeout=15):
        # The Editor maximizes itself once it's up, and coordinates aren't final until the compositor has done it
        self._wait_ready()
        deadline = time.monotonic() + timeout
        while True:
            info = self.request("info")
            if info["maximized"] or time.monotonic() > deadline:
                return "%dx%d%s" % (info["width"], info["height"], "" if info["maximized"] else " (not maximized)")
            time.sleep(0.1)

    def cmd_click(self, x, y, button="left", count=1):
        self.request("dblclick" if count == 2 else "click", x=x, y=y, button=button)
        return "clicked (%d, %d)" % (x, y)

    def cmd_move(self, x, y):
        self.request("move", x=x, y=y)
        return "moved to (%d, %d)" % (x, y)

    def cmd_drag(self, x, y, x2, y2, button="left", steps=10):
        if button != "left":
            raise DriverError("The harness drags with the left button only")
        self.request("drag", x=x, y=y, x2=x2, y2=y2)
        return "dragged (%d, %d) -> (%d, %d)" % (x, y, x2, y2)

    def cmd_scroll(self, x, y, notches):
        self.request("scroll", x=x, y=y, notches=notches)
        return "scrolled %d at (%d, %d)" % (notches, x, y)

    def cmd_key(self, keys):
        self.request("key", keys=keys)
        return "typed %s" % json.dumps(keys)

    def cmd_shot(self, out):
        reply = self.request("shot", path=out)
        return "%s %dx%d (the frame itself, from the harness)" % (reply["path"], reply["width"], reply["height"])

    def cmd_info(self):
        info = self.request("info")
        return "%s client=%dx%d frame=%d (harness; real input %s)" % (
            info["title"], info["width"], info["height"], info["frame"], "blocked" if info["realInputBlocked"] else "allowed")

    def cmd_state(self):
        reply = self.request("state")
        reply.pop("id", None)
        reply.pop("ok", None)
        return json.dumps(reply, indent=1)

    def cmd_object(self, name):
        reply = self.request("object", name=name)
        reply.pop("id", None)
        reply.pop("ok", None)
        return json.dumps(reply, indent=1)

    def cmd_realinput(self, on):
        self.request("realinput", on=on)
        return "real input %s" % ("allowed" if on else "blocked")

    def cmd_close(self, timeout):
        if self.exit_code is None:
            # As the close button does: unsaved changes are asked about first
            self.request("quit")
            deadline = time.monotonic() + timeout
            while self.exit_code is None and time.monotonic() < deadline:
                time.sleep(0.1)
            if self.exit_code is None:
                raise DriverError("Still running after %ds (an unsaved-changes prompt? see `shot`), then `quit`" % timeout)
        self._end_group()
        return "closed code=%d%s" % (self.exit_code, self._restore_note())


def renderdoc_captures():
    if not os.path.isdir(RENDERDOC_DIR):
        return []
    return sorted(os.path.join(RENDERDOC_DIR, f) for f in os.listdir(RENDERDOC_DIR) if f.endswith(".rdc"))


def file_settled(path):
    """Whether RenderDoc has finished writing path: its size holds for a moment."""
    size = os.path.getsize(path)
    time.sleep(0.5)
    return size > 0 and os.path.getsize(path) == size


def summarize_capture(path):
    """The capture's thumbnail beside it, and what it holds, counted from RenderDoc's XML export."""
    thumbnail = os.path.splitext(path)[0] + ".png"
    subprocess.run(["renderdoccmd", "thumb", "--out", thumbnail, path], check=True,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    with tempfile.TemporaryDirectory() as folder:
        xml = os.path.join(folder, "capture.xml")
        subprocess.run(["renderdoccmd", "convert", "-f", path, "-o", xml, "-c", "xml"], check=True,
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        import xml.etree.ElementTree as ElementTree
        names = [chunk.get("name") or "" for chunk in ElementTree.parse(xml).getroot().iter("chunk")]
    counts = {
        "draws": sum(1 for n in names if n.startswith("glDraw") or n.startswith("glMultiDraw")),
        "dispatches": sum(1 for n in names if n.startswith("glDispatchCompute")),
        "debug groups": sum(1 for n in names if n == "glPushDebugGroup"),
        "GL calls": len(names),
    }
    return "%s (%d KB), thumbnail %s: %s" % (path, os.path.getsize(path) // 1024, thumbnail,
                                              ", ".join("%s=%d" % kv for kv in counts.items()))


def png_size(path):
    with open(path, "rb") as f:
        header = f.read(24)
    return list(struct.unpack(">II", header[16:24]))


def run_helper(options):
    os.makedirs(STATE_DIR, exist_ok=True)
    helper = HarnessHelper(options) if options.get("harness") else Helper(options)
    if os.path.exists(SOCKET_PATH):
        os.unlink(SOCKET_PATH)
    server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    server.bind(SOCKET_PATH)
    server.listen(4)
    stopping = False
    try:
        while not stopping:
            connection, _ = server.accept()
            with connection:
                request = json.loads(read_message(connection))
                command = request.pop("command")
                try:
                    result = {"ok": True, "text": getattr(helper, "cmd_" + command)(**request)}
                except DriverError as error:
                    result = {"ok": False, "text": str(error)}
                except Exception as error:  # Reported, and the session goes on
                    result = {"ok": False, "text": "%s: %s" % (type(error).__name__, error)}
                connection.sendall(json.dumps(result).encode() + b"\n")
                stopping = command in ("quit", "close") and result["ok"] or command == "stop"
    finally:
        # The socket goes first, so no command reaches a helper that's stopping
        server.close()
        os.unlink(SOCKET_PATH)
        helper.shutdown()


def read_message(connection):
    data = b""
    while not data.endswith(b"\n"):
        chunk = connection.recv(65536)
        if not chunk:
            break
        data += chunk
    return data.decode()


def send(command, **arguments):
    if not os.path.exists(SOCKET_PATH):
        raise DriverError("Nothing launched. Run: driver.py launch")
    connection = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    try:
        connection.connect(SOCKET_PATH)
    except (ConnectionRefusedError, FileNotFoundError):
        os.unlink(SOCKET_PATH)
        raise DriverError("The last session's helper is gone. Run: driver.py launch")
    with connection:
        try:
            connection.sendall(json.dumps(dict(arguments, command=command)).encode() + b"\n")
            result = json.loads(read_message(connection))
        except (ConnectionResetError, BrokenPipeError, json.JSONDecodeError):
            raise DriverError("The session's helper stopped (the app was closed or quit)")
    if not result["ok"]:
        raise DriverError(result["text"])
    return result["text"]


def helper_running():
    try:
        send("alive")
        return True
    except DriverError:
        return False


# ---------------------------------------------------------------------------------------------------------------------
# Optick: a capture over its network protocol (ThirdParty/Include/Optick/optick_message.*), as Optick's viewer takes
# one. Every message is little-endian.

OPTICK_PORT = 31318
OPTICK_MARK = 0xB50FB50F
OPTICK_APP = 0xB50F
OPTICK_START, OPTICK_STOP = 0, 1
OPTICK_INSTRUMENTATION = 0x3
OPTICK_RESPONSES = {0: "FrameDescriptionBoard", 1: "EventFrame", 2: "SamplingFrame", 3: "NullFrame", 4: "ReportProgress",
                    5: "Handshake", 7: "SynchronizationData", 8: "TagsPack", 9: "CallstackDescriptionBoard",
                    10: "CallstackPack", 256: "FiberSynchronizationData", 257: "SyscallPack", 258: "SummaryPack",
                    259: "FramesPack"}


def optick_message(message_type, payload=b""):
    body = struct.pack("<HH", OPTICK_APP, message_type) + payload
    return struct.pack("<II", OPTICK_MARK, len(body)) + body


def optick_capture(seconds, out_path):
    connection = None
    for port in range(OPTICK_PORT, OPTICK_PORT + 4):  # The server binds the first free one of 4
        try:
            connection = socket.create_connection(("127.0.0.1", port), timeout=5)
            break
        except OSError:
            continue
    if connection is None:
        raise DriverError("Nothing is listening on ports %d-%d (is a Debug or Development build running?)" % (OPTICK_PORT, OPTICK_PORT + 3))
    # mode, category mask, sampling frequency, frame limit, time limit, spike limit (u32 each), memory limit (u64),
    # then the root password, an empty string (an i32 length)
    settings = struct.pack("<IIIIIIQi", OPTICK_INSTRUMENTATION, 0xFFFFFFFF, 0, 0, 0, 0, 0, 0)
    counts, total, raw = {}, 0, bytearray()
    with connection:
        connection.sendall(optick_message(OPTICK_START, settings))
        time.sleep(seconds)
        connection.sendall(optick_message(OPTICK_STOP))
        connection.settimeout(30)
        buffer = b""
        finished = False
        while not finished:
            chunk = connection.recv(1 << 20)
            if not chunk:
                break
            buffer += chunk
            raw += chunk
            while len(buffer) >= 12:
                version, size, response_type, app = struct.unpack("<IIHH", buffer[:12])
                if version != 25 or app != OPTICK_APP:
                    raise DriverError("Not Optick's protocol 25 (version %d, app %#x)" % (version, app))
                if len(buffer) < 12 + size:
                    break
                name = OPTICK_RESPONSES.get(response_type, str(response_type))
                counts[name] = counts.get(name, 0) + 1
                total += size
                buffer = buffer[12 + size:]
                if name == "NullFrame":
                    finished = True
    if out_path:
        with open(out_path, "wb") as f:
            f.write(raw)
    if not counts.get("EventFrame"):
        raise DriverError("The capture had no event frames: %s" % counts)
    return "captured %d bytes: %s%s" % (total, ", ".join("%s=%d" % kv for kv in sorted(counts.items())),
                                         ("; raw stream in " + out_path) if out_path else ""), bytes(raw)


class OptickReader:
    """Reads the fields of one response's payload, as Optick's OutputDataStream wrote them (optick_serialization.cpp)"""

    def __init__(self, data):
        self.data, self.at = data, 0

    def take(self, fmt):
        values = struct.unpack_from("<" + fmt, self.data, self.at)
        self.at += struct.calcsize("<" + fmt)
        return values if len(values) > 1 else values[0]

    def string(self):
        length = self.take("I")
        text = self.data[self.at:self.at + length].decode("utf-8", "replace")
        self.at += length
        return text


def optick_decode(raw):
    """The main thread's frames and scopes from a raw capture stream: (frequency, frames, events), where frames are
    (start, finish) ticks of each CPU frame and events (start, finish, name) of every main-thread scope"""
    frequency, main_thread, names, frames, events_by_thread = None, None, [], [], {}
    at = 0
    while at + 12 <= len(raw):
        _, size, response_type, _ = struct.unpack_from("<IIHH", raw, at)
        payload = raw[at + 12:at + 12 + size]
        at += 12 + size
        reader = OptickReader(payload)
        if response_type == 0:  # FrameDescriptionBoard: Core::DumpBoard
            reader.take("I")  # board number
            frequency = reader.take("q")
            reader.take("QI")  # origin, precision
            reader.take("qq")  # time slice
            for _ in range(reader.take("I")):  # threads: id, process id, name, max depth, priority, mask
                reader.take("QI")
                reader.string()
                reader.take("iiI")
            for _ in range(reader.take("I")):  # fibers
                reader.take("Q")
            main_thread = reader.take("I")
            for _ in range(reader.take("I")):  # event descriptions, in index order
                name = reader.string()
                reader.string()  # file
                reader.take("IIIfB")  # line, filter, colour, budget, flags
                names.append(name)
        elif response_type == 259:  # FramesPack: frames of each FrameType, CPU first
            reader.take("I")  # board number
            if reader.take("I") > 0:
                for _ in range(reader.take("I")):
                    start, finish, _, _ = reader.take("qqIQ")
                    frames.append((start, finish))
        elif response_type == 1:  # EventFrame: ScopeData, a header then its categories and events
            _, thread, _, _, _ = reader.take("Iiiqq")
            # Categories, a copy of the coloured events. Not "at += take()": that reads at before take moves it.
            categories = reader.take("I")
            reader.at += categories * 20
            thread_events = events_by_thread.setdefault(thread, [])
            for _ in range(reader.take("I")):
                thread_events.append(reader.take("qqI"))
    if frequency is None or not frames:
        raise DriverError("The capture had no description board or no frames")
    events = [(start, finish, names[index] if index < len(names) else "?")
              for start, finish, index in events_by_thread.get(main_thread, []) if finish >= start]
    return frequency, sorted(frames), events


def optick_report(raw, scopes, subtract, top):
    """Per frame: its time, the time in each of scopes, and the frame less the time in subtract; then the main
    thread's top scopes by time a frame. A scope given matches every scope whose name contains it, since OPTICK_EVENT()
    names a scope by its whole signature, which differs between compilers ("void GameScene::Update()" with clang,
    "void __cdecl GameScene::Update(void)" with MSVC): "GameScene::Update(" picks out one function. Scopes inside
    another that matches the same name count once."""
    frequency, frames, events = optick_decode(raw)
    to_ms = 1000.0 / frequency
    starts = [start for start, _ in frames]
    wanted = list(dict.fromkeys(scopes + subtract))
    matches = {}

    # Each event's frame, by its start; time in a name counts only its outermost scopes
    per_frame = [dict() for _ in frames]
    totals = {}
    open_until = {}
    for start, finish, name in sorted(events):
        frame = bisect.bisect_right(starts, start) - 1
        if frame < 0 or start > frames[frame][1]:
            continue
        if name not in matches:
            matches[name] = [scope for scope in wanted if scope in name]
        duration = (finish - start) * to_ms
        for key in [name] + matches[name]:
            if open_until.get(key, -1) > start:
                continue
            open_until[key] = finish
            per_frame[frame][key] = per_frame[frame].get(key, 0.0) + duration
            if key == name:
                totals[name] = totals.get(name, 0.0) + duration

    def stats(values):
        ordered = sorted(values)
        count = len(ordered)
        return "mean %.2f  median %.2f  p95 %.2f  p99 %.2f ms" % (
            sum(ordered) / count, ordered[count // 2], ordered[min(count - 1, int(count * 0.95))],
            ordered[min(count - 1, int(count * 0.99))])

    # The first and last frames are cut by the capture's start and stop
    kept = range(1, len(frames) - 1) if len(frames) > 2 else range(len(frames))
    frame_ms = [(frames[i][1] - frames[i][0]) * to_ms for i in kept]
    lines = ["%d frames: %s" % (len(frame_ms), stats(frame_ms))]
    for name in scopes:
        values = [per_frame[i].get(name, 0.0) for i in kept]
        share = 100.0 * sum(values) / sum(frame_ms)
        lines.append("%s: %s, %.1f%% of frame time, in %d frames" % (
            name, stats(values), share, sum(1 for v in values if v > 0)))
    if subtract:
        values = [frame_ms[n] - sum(per_frame[i].get(name, 0.0) for name in subtract) for n, i in enumerate(kept)]
        lines.append("frame less %s: %s" % (" and ".join(subtract), stats(values)))
        for name in scopes:
            if name in subtract:
                continue
            share = 100.0 * sum(per_frame[i].get(name, 0.0) for i in kept) / sum(values)
            lines.append("%s: %.1f%% of frame time less %s" % (name, share, " and ".join(subtract)))
    if top:
        lines.append("main thread's top scopes, ms a frame:")
        for name, total in sorted(totals.items(), key=lambda item: -item[1])[:top]:
            lines.append("  %8.3f  %s" % (total / len(frames), name))
    return "\n".join(lines)


# ---------------------------------------------------------------------------------------------------------------------

def command_launch(args):
    if args.harness:
        if args.app != "Editor" or args.renderdoc:
            raise DriverError("--harness is the Editor's, and RenderDoc's capture key needs real keys: use one or the other")
        args.app = "EditorHarness"
    if helper_running():
        raise DriverError("A session is running. Run: driver.py quit (or close)")
    if subprocess.run(["pgrep", "-x", args.app], capture_output=True).returncode == 0:
        raise DriverError("%s is already running outside the driver. Close it first." % args.app)
    if args.nested and not args.harness:
        raise DriverError("--nested is for --harness sessions: nothing gives the nested KWin real input devices")
    if args.nested and args.output:
        raise DriverError("--nested has one output of its own; --output is for the desktop's monitors")
    if args.nested and (shutil.which("kwin_wayland") is None or shutil.which("kscreen-doctor") is None):
        raise DriverError("--nested needs kwin_wayland and kscreen-doctor (KDE Plasma)")
    if args.refresh is not None and (not args.nested or args.refresh <= 0):
        raise DriverError("--refresh is the nested output's rate: use it with --nested, above 0")
    if args.output:
        names = [o["name"] for o in kwin_outputs()]
        if args.output not in names:
            raise DriverError("No output called %s; KWin has: %s" % (args.output, ", ".join(names)))
    if args.renderdoc and shutil.which("renderdoccmd") is None:
        raise DriverError("renderdoccmd isn't installed: sudo dnf install renderdoc")
    exe = os.path.join(ENGINE_ROOT, "_build", args.config, args.app)
    if not os.path.exists(exe):
        raise DriverError("%s isn't built. Build it first (see SKILL.md)." % exe)
    os.makedirs(STATE_DIR, exist_ok=True)
    app_args = (["-scene", args.scene] if args.scene else []) + args.extra
    options = {"app": args.app, "config": args.config, "args": app_args, "renderdoc": args.renderdoc,
               "keep_imgui_ini": args.keep_imgui_ini, "harness": args.harness, "output": args.output,
               "nested": args.nested, "refresh": args.refresh}
    helper = subprocess.Popen([sys.executable, "-I", os.path.abspath(__file__), "_helper", json.dumps(options)],
                              stdout=open(HELPER_LOG, "wb"), stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL,
                              start_new_session=True)
    # The helper is up once its socket answers; the app, once its main window has its title
    deadline = time.monotonic() + args.timeout
    while not helper_running():
        if helper.poll() is not None:
            with open(HELPER_LOG, errors="replace") as f:
                lines = f.read().strip().splitlines()
            raise DriverError("The helper failed to start: %s (all of it in %s)" % (lines[-1] if lines else "?", HELPER_LOG))
        if time.monotonic() > deadline:
            raise DriverError("The helper didn't start; see %s" % HELPER_LOG)
        time.sleep(0.1)
    title_prefix = "RZEStudio -" if args.app in ("Editor", "EditorHarness") else ""
    while True:
        state = send("alive")
        if state.startswith("exited"):
            raise DriverError("%s %s; see `driver.py log`" % (args.app, state))
        try:
            info = send("info")
            if info.startswith(title_prefix):
                break
        except DriverError:
            pass
        if time.monotonic() > deadline:
            raise DriverError("No window after %ds; see `driver.py log`" % args.timeout)
        time.sleep(0.25)
    send("wait_settled")
    time.sleep(args.settle)
    restored_leftover = send("restored_leftover_ini") == "True"
    if args.keep_imgui_ini:
        note = " (restored an imgui.ini backup a session left; this run keeps its layout)" if restored_leftover else ""
    else:
        note = " (imgui.ini backed up%s)" % ("; restored a backup a session left" if restored_leftover else "")
    if args.nested:
        note += " (nested: %s, off-screen)" % send("nested_mode")
    if send("stopped_leftover_nested") == "True":
        note += " (ended a nested KWin a session left)"
    return "launched: " + send("info") + note


def default_shot_path():
    return os.path.join(SHOT_DIR, datetime.datetime.now().strftime("%Y%m%d-%H%M%S-%f") + ".png")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)

    p = commands.add_parser("launch", help="start the Editor (or Game) under the driver")
    p.add_argument("--scene", help="e.g. DrawLineTest.scene")
    p.add_argument("--config", default="development", choices=["debug", "development", "retail"])
    p.add_argument("--app", default="Editor", choices=["Editor", "Game"])
    p.add_argument("--settle", type=float, default=5, help="seconds to wait after the window is up (assets stream in)")
    p.add_argument("--timeout", type=float, default=60)
    p.add_argument("--renderdoc", action="store_true", help="run under renderdoccmd, for `renderdoc-capture`")
    p.add_argument("--keep-imgui-ini", action="store_true",
                   help="keep the layout the run leaves; by default Config/imgui.ini is put back as it was")
    p.add_argument("--harness", action="store_true",
                   help="run EditorHarness: input and shots go through the app, leaving the real cursor and focus alone")
    p.add_argument("--output", help="the monitor to open on, by KWin's name for it (e.g. DP-2; see `outputs`)")
    p.add_argument("--nested", action="store_true",
                   help="with --harness: run in a KWin of the session's own, off-screen, so nothing reaches the desktop")
    p.add_argument("--refresh", type=int, help="with --nested: the off-screen output's refresh rate (default %d Hz)"
                   % NESTED_REFRESH_HZ)
    p.add_argument("extra", nargs="*", help="more arguments for the app, after --")
    commands.add_parser("outputs", help="the monitors launch --output takes: name, then x,y widthxheight")

    p = commands.add_parser("shot", help="screenshot the client area")
    p.add_argument("--out")
    for name, extra in (("click", True), ("dblclick", False), ("move", False)):
        p = commands.add_parser(name)
        p.add_argument("x", type=int)
        p.add_argument("y", type=int)
        if extra:
            p.add_argument("--button", default="left", choices=list(BUTTONS))
    p = commands.add_parser("drag")
    for axis in ("x", "y", "x2", "y2"):
        p.add_argument(axis, type=int)
    p.add_argument("--button", default="left", choices=list(BUTTONS))
    p = commands.add_parser("scroll", help="mouse wheel: + away from you, - towards")
    p.add_argument("x", type=int)
    p.add_argument("y", type=int)
    p.add_argument("notches", type=int)
    p = commands.add_parser("key", help="SendKeys syntax, e.g. '^a45{ENTER}'")
    p.add_argument("keys")
    commands.add_parser("info")
    commands.add_parser("alive")
    p = commands.add_parser("windows", help="windows opened since launch (e.g. a file manager the app opened)")
    p.add_argument("--all", action="store_true")
    p = commands.add_parser("close-window", help="close a window opened since launch, by caption")
    p.add_argument("caption")
    p = commands.add_parser("close", help="close the window as a user would, and check the exit code")
    p.add_argument("--timeout", type=int, default=30)
    commands.add_parser("quit", help="kill the app")
    p = commands.add_parser("log", help="the app's output")
    p.add_argument("-n", type=int, default=40)
    p.add_argument("--engine", action="store_true", help="the newest RZE/Logs/RZELOG_*.txt instead, the engine's own log")
    p.add_argument("--grep")
    p = commands.add_parser("clip", help="the clipboard: clip get, clip set TEXT")
    p.add_argument("action", choices=["get", "set"])
    p.add_argument("text", nargs="?", default="")
    p = commands.add_parser("optick", help="take an Optick capture over the network, as Optick's viewer does")
    p.add_argument("--seconds", type=float, default=2)
    p.add_argument("--out", help="also save the raw stream here")
    p.add_argument("--scopes", default="", help="comma-separated scope names to time a frame, e.g. GameScene::Update")
    p.add_argument("--subtract", default="", help="comma-separated scope names whose time to take off each frame's")
    p.add_argument("--top", type=int, default=0, help="also list the main thread's N costliest scopes")
    commands.add_parser("state", help="the Editor's state: scene, modified, selection, undo, objects (--harness)")
    p = commands.add_parser("object", help="an object's components and transform (--harness)")
    p.add_argument("name")
    p = commands.add_parser("realinput", help="let the real mouse and keyboard reach the app, or block them again (--harness)")
    p.add_argument("state", choices=["on", "off"])
    p = commands.add_parser("renderdoc-capture", help="capture the next frame with RenderDoc (launch --renderdoc)")
    p.add_argument("--timeout", type=int, default=30)

    if len(sys.argv) > 1 and sys.argv[1] == "_helper":
        run_helper(json.loads(sys.argv[2]))
        return 0
    args = parser.parse_args()

    try:
        c = args.command
        if c == "launch":
            print(command_launch(args))
        elif c == "outputs":
            print(describe_outputs())
        elif c == "shot":
            print(send("shot", out=os.path.abspath(args.out) if args.out else default_shot_path()))
        elif c == "click":
            print(send("click", x=args.x, y=args.y, button=args.button))
        elif c == "dblclick":
            print(send("click", x=args.x, y=args.y, count=2))
        elif c == "move":
            print(send("move", x=args.x, y=args.y))
        elif c == "drag":
            print(send("drag", x=args.x, y=args.y, x2=args.x2, y2=args.y2, button=args.button))
        elif c == "scroll":
            print(send("scroll", x=args.x, y=args.y, notches=args.notches))
        elif c == "key":
            print(send("key", keys=args.keys))
        elif c in ("info", "alive", "quit"):
            print("not running" if c == "alive" and not helper_running() else send(c))
        elif c == "state":
            print(send("state"))
        elif c == "object":
            print(send("object", name=args.name))
        elif c == "realinput":
            print(send("realinput", on=args.state == "on"))
        elif c == "windows":
            print(send("windows", all_windows=args.all))
        elif c == "close-window":
            print(send("close_window", caption=args.caption))
        elif c == "close":
            print(send("close", timeout=args.timeout))
        elif c == "log":
            path = APP_LOG
            if args.engine:
                logs = [os.path.join(ENGINE_LOGS, f) for f in os.listdir(ENGINE_LOGS) if f.startswith("RZELOG_")]
                if not logs:
                    raise DriverError("No logs in %s" % ENGINE_LOGS)
                path = max(logs, key=os.path.getmtime)
                print("(%s)" % os.path.relpath(path, REPO))
            with open(path, errors="replace") as f:
                lines = f.read().splitlines()
            if args.grep:
                lines = [l for l in lines if args.grep.lower() in l.lower()]
            print("\n".join(lines[-args.n:]))
        elif c == "clip":
            # The app's clipboard: a nested session's, or the desktop's
            environment = dict(os.environ)
            display = send("wayland_display") if helper_running() else ""
            if display:
                environment["WAYLAND_DISPLAY"] = display
            if args.action == "get":
                print(subprocess.run(["wl-paste", "--no-newline"], capture_output=True, text=True, env=environment).stdout)
            else:
                # wl-copy stays behind to serve the text; a nested session's compositor ending ends it, noisily
                subprocess.run(["wl-copy", args.text], check=True, env=environment,
                               stderr=subprocess.DEVNULL if display else None)
                print("copied %d characters" % len(args.text))
        elif c == "optick":
            summary, raw = optick_capture(args.seconds, args.out)
            print(summary)
            scopes = [name for name in args.scopes.split(",") if name]
            subtract = [name for name in args.subtract.split(",") if name]
            if scopes or subtract or args.top:
                print(optick_report(raw, scopes + [name for name in subtract if name not in scopes], subtract, args.top))
        elif c == "renderdoc-capture":
            print(summarize_capture(send("renderdoc_capture", timeout=args.timeout)))
    except DriverError as error:
        print("error: %s" % error, file=sys.stderr)
        return 1
    return 0



if __name__ == "__main__":
    sys.exit(main())
