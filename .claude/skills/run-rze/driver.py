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
import datetime
import fcntl
import json
import os
import shutil
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
ENGINE_LOGS = os.path.join(ENGINE_ROOT, "Logs")
SHOT_DIR = os.path.join(tempfile.gettempdir(), "rze-shots")


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


def kwin(body, timeout_ms=3000):
    """Runs body as a KWin script; it calls report(value) once, and that value is returned."""
    from gi.repository import Gio, GLib

    bus = Gio.bus_get_sync(Gio.BusType.SESSION)
    result = {}
    loop = GLib.MainLoop()

    def on_call(_conn, _sender, _path, _iface, _method, params, invocation):
        result["json"] = params.unpack()[0]
        invocation.return_value(None)
        loop.quit()

    node = Gio.DBusNodeInfo.new_for_xml(KWIN_REPLY_XML)
    registration = bus.register_object("/", node.interfaces[0], on_call, None, None)
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
    if "json" not in result:
        raise DriverError("KWin didn't answer (is this a KDE Plasma session?)")
    return json.loads(result["json"])


def kwin_windows():
    return kwin("var out = []; workspace.windowList().forEach(function (w) { out.push(describe(w)); }); report(out);")


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
# The helper: one per launched session

def restore_imgui_ini():
    """Puts back the layout a session found, if it has a backup. A copy, so the file gets a new timestamp."""
    if not os.path.exists(IMGUI_INI_BACKUP):
        return False
    shutil.copyfile(IMGUI_INI_BACKUP, IMGUI_INI)
    os.remove(IMGUI_INI_BACKUP)
    return True


def back_up_imgui_ini():
    restored = restore_imgui_ini()
    if os.path.exists(IMGUI_INI):
        shutil.copyfile(IMGUI_INI, IMGUI_INI_BACKUP)
    return restored


class Helper:
    def __init__(self, options):
        self.options = options
        self.restored_leftover_ini = False if options.get("keep_imgui_ini") else back_up_imgui_ini()
        self.lock = threading.Lock()
        self.exit_code = None
        self.window_id = None
        self.windows_at_launch = {w["id"] for w in kwin_windows()}
        self.origin_size = kwin_desktop_size()
        self.pointer = VirtualDevice("RZE driver pointer", keys=BUTTONS.values(), abs_size=self.origin_size, wheel=True)
        self.keyboard = VirtualDevice("RZE driver keyboard", keys=list(range(1, 128)))
        self.devices_ready_at = time.monotonic() + 0.8

        build_dir = os.path.join(ENGINE_ROOT, "_build", options["config"])
        exe = os.path.join(build_dir, options["app"])
        command = ([] if not options.get("renderdoc") else
                   ["renderdoccmd", "capture", "--wait-for-exit", "--working-dir", build_dir,
                    "--capture-file", os.path.join(STATE_DIR, "capture")])
        # Line-buffered, so `log` sees each line as it's written
        command += ["stdbuf", "-oL", "-eL", exe] + options["args"]
        self.process = subprocess.Popen(command, cwd=build_dir, stdout=open(APP_LOG, "wb"), stderr=subprocess.STDOUT,
                                        stdin=subprocess.DEVNULL)
        threading.Thread(target=self._wait_for_exit, daemon=True).start()

    def _wait_for_exit(self):
        self.exit_code = self.process.wait()

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
        return "closed code=%d%s" % (self.exit_code, self._restore_note())

    def cmd_quit(self):
        if self.exit_code is None:
            self.process.kill()
            self.process.wait()
        return "quit" + self._restore_note()

    # Once the app has exited, since ImGui saves the layout as it shuts down
    def _restore_note(self):
        return "; imgui.ini restored" if restore_imgui_ini() else ""

    def shutdown(self):
        if self.exit_code is None:
            self.process.kill()
            self.process.wait()
        restore_imgui_ini()
        self.pointer.close()
        self.keyboard.close()


def png_size(path):
    with open(path, "rb") as f:
        header = f.read(24)
    return list(struct.unpack(">II", header[16:24]))


def run_helper(options):
    os.makedirs(STATE_DIR, exist_ok=True)
    helper = Helper(options)
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
        raise DriverError("Nothing is listening on ports %d-%d (is a Debug or Release build running?)" % (OPTICK_PORT, OPTICK_PORT + 3))
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
                                         ("; raw stream in " + out_path) if out_path else "")


# ---------------------------------------------------------------------------------------------------------------------

def command_launch(args):
    if helper_running():
        raise DriverError("A session is running. Run: driver.py quit (or close)")
    if subprocess.run(["pgrep", "-x", args.app], capture_output=True).returncode == 0:
        raise DriverError("%s is already running outside the driver. Close it first." % args.app)
    if args.renderdoc and shutil.which("renderdoccmd") is None:
        raise DriverError("renderdoccmd isn't installed: sudo dnf install renderdoc")
    exe = os.path.join(ENGINE_ROOT, "_build", args.config, args.app)
    if not os.path.exists(exe):
        raise DriverError("%s isn't built. Build it first (see SKILL.md)." % exe)
    os.makedirs(STATE_DIR, exist_ok=True)
    app_args = (["-scene", args.scene] if args.scene else []) + args.extra
    options = {"app": args.app, "config": args.config, "args": app_args, "renderdoc": args.renderdoc,
               "keep_imgui_ini": args.keep_imgui_ini}
    subprocess.Popen([sys.executable, "-I", os.path.abspath(__file__), "_helper", json.dumps(options)],
                     stdout=open(HELPER_LOG, "wb"), stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL,
                     start_new_session=True)
    # The helper is up once its socket answers; the app, once its main window has its title
    deadline = time.monotonic() + args.timeout
    while not helper_running():
        if time.monotonic() > deadline:
            raise DriverError("The helper didn't start; see %s" % HELPER_LOG)
        time.sleep(0.1)
    title_prefix = "RZEStudio -" if args.app == "Editor" else ""
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
    time.sleep(args.settle)
    note = "" if args.keep_imgui_ini else " (imgui.ini backed up%s)" % (
        "; restored a backup a session left" if send("restored_leftover_ini") == "True" else "")
    return "launched: " + send("info") + note


def default_shot_path():
    return os.path.join(SHOT_DIR, datetime.datetime.now().strftime("%Y%m%d-%H%M%S-%f") + ".png")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)

    p = commands.add_parser("launch", help="start the Editor (or Game) under the driver")
    p.add_argument("--scene", help="e.g. DrawLineTest.scene")
    p.add_argument("--config", default="debug", choices=["debug", "release", "retail"])
    p.add_argument("--app", default="Editor", choices=["Editor", "Game"])
    p.add_argument("--settle", type=float, default=5, help="seconds to wait after the window is up (assets stream in)")
    p.add_argument("--timeout", type=float, default=60)
    p.add_argument("--renderdoc", action="store_true", help="run under renderdoccmd, for `renderdoc-capture`")
    p.add_argument("--keep-imgui-ini", action="store_true",
                   help="keep the layout the run leaves; by default Config/imgui.ini is put back as it was")
    p.add_argument("extra", nargs="*", help="more arguments for the app, after --")

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
    commands.add_parser("renderdoc-capture", help="take a RenderDoc capture of the next frame (launch --renderdoc)")

    if len(sys.argv) > 1 and sys.argv[1] == "_helper":
        run_helper(json.loads(sys.argv[2]))
        return 0
    args = parser.parse_args()

    try:
        c = args.command
        if c == "launch":
            print(command_launch(args))
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
            if args.action == "get":
                print(subprocess.run(["wl-paste", "--no-newline"], capture_output=True, text=True).stdout)
            else:
                subprocess.run(["wl-copy", args.text], check=True)
                print("copied %d characters" % len(args.text))
        elif c == "optick":
            print(optick_capture(args.seconds, args.out))
        elif c == "renderdoc-capture":
            print(renderdoc_capture())
    except DriverError as error:
        print("error: %s" % error, file=sys.stderr)
        return 1
    return 0


def renderdoc_capture():
    raise DriverError("Not written yet: needs renderdoc installed (sudo dnf install renderdoc)")


if __name__ == "__main__":
    sys.exit(main())
