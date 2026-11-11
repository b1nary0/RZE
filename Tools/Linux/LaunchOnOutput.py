#!/usr/bin/env python3
"""Runs a program with its windows on one monitor, on KDE Plasma (KWin), for the run-rze harness.

Wayland lets no application place its own windows, so the engine's -launchMonitor does nothing there. KWin runs
scripts handed to it over D-Bus, and a KWin script can move any window. This loads one that sends the program's windows
to the output named, waits until KWin has it running, then runs the program in this process: the program keeps this
PID, which is what the script matches, and whoever started this sees the program as its own child.

The script moves each window the program opens until it has moved a normal, resizable one: the Editor's or Game's main
window. The splash, which isn't resizable, comes and goes before it. Windows after that are left where they open. A window
it moved that leaves the output in the next few seconds is sent back: on X11 (XWayland, as under RenderDoc) the program
places its own windows, on its -launchMonitor, after they're shown. A watcher process unloads the script those seconds
after the main window, after --timeout, or once the program has exited, whichever comes first.

Usage:
  LaunchOnOutput.py --list
  LaunchOnOutput.py --output DP-2 [--timeout 120] [--log file] -- program [args...]
"""

import argparse
import json
import os
import sys
import tempfile
import time

REPLY_INTERFACE = "org.rze.LaunchOnOutput"
REPLY_XML = ("<node><interface name='%s'><method name='Report'>"
             "<arg type='s' name='json' direction='in'/></method></interface></node>" % REPLY_INTERFACE)
# KWin runs a script within milliseconds; this is for one that never answers
READY_TIMEOUT_S = 5
# How long after the main window appears the script keeps the program's windows on the output (KWin scripts have no
# timers, so the watcher ends it by unloading the script)
SETTLE_S = 3
POLL_MS = 250

# A script's print() is filtered out of KWin's log, so it reports with callDBus to a name the watcher owns
KWIN_SCRIPT = """
function report(event, detail) {
    callDBus(%(bus)s, '/', %(interface)s, 'Report', JSON.stringify({ event: event, detail: detail }));
}
var outputs = workspace.screens.map(function (s) {
    return { name: s.name, geometry: [s.geometry.x, s.geometry.y, s.geometry.width, s.geometry.height] };
});
var target = null;
workspace.screens.forEach(function (s) { if (s.name == %(output)s) target = s; });
function keepOnTarget(w) {
    w.outputChanged.connect(function () {
        if (w.output == target) return;
        report("sentBack", { caption: w.caption, from: w.output.name });
        workspace.sendClientToScreen(w, target);
    });
}
function place(w) {
    if (w.pid != %(pid)d) return;
    var from = w.output.name;
    if (w.output != target) workspace.sendClientToScreen(w, target);
    keepOnTarget(w);
    // The splash is a normal window too (Wayland has no utility windows), but the main window is the one that resizes
    var isMain = w.normalWindow && !w.utility && w.resizeable;
    report("moved", { caption: w.caption, main: isMain, from: from, to: w.output.name, resizeable: w.resizeable,
                      frame: [w.frameGeometry.x, w.frameGeometry.y, w.frameGeometry.width, w.frameGeometry.height] });
    if (isMain) {
        workspace.windowAdded.disconnect(place);
        report("main", null);
    }
}
if (%(list)s) {
    report("outputs", outputs);
} else if (target == null) {
    report("error", { message: "no output called " + %(output)s, outputs: outputs });
} else {
    workspace.windowAdded.connect(place);
    report("ready", null);
}
"""


class Log:
    def __init__(self, path):
        self.file = open(path, "a") if path else None

    def write(self, text):
        if self.file:
            self.file.write("%s [%d] %s\n" % (time.strftime("%H:%M:%S"), os.getpid(), text))
            self.file.flush()


def run_kwin_script(app_pid, output, is_list, on_report, keep_going, log):
    """Loads the script and runs a main loop until on_report or keep_going (polled) returns False. Always unloads it."""
    from gi.repository import Gio, GLib

    bus = Gio.bus_get_sync(Gio.BusType.SESSION)
    loop = GLib.MainLoop()

    def on_call(_conn, _sender, _path, _iface, _method, params, invocation):
        invocation.return_value(None)
        report = json.loads(params.unpack()[0])
        log.write("kwin: %s" % json.dumps(report))
        if not on_report(report):
            loop.quit()

    def poll():
        if keep_going():
            return True
        loop.quit()
        return False

    # Bus names can't have an element that starts with a digit
    bus_name = "%s.p%d" % (REPLY_INTERFACE, app_pid)
    script_name = "rze_launch_on_output_%d" % app_pid
    node = Gio.DBusNodeInfo.new_for_xml(REPLY_XML)
    registration = bus.register_object_with_closures2("/", node.interfaces[0], on_call, None, None)
    owner = Gio.bus_own_name_on_connection(bus, bus_name, Gio.BusNameOwnerFlags.NONE, None, None)
    script = KWIN_SCRIPT % {"bus": json.dumps(bus_name), "interface": json.dumps(REPLY_INTERFACE),
                            "output": json.dumps(output or ""), "pid": app_pid, "list": "true" if is_list else "false"}
    fd, path = tempfile.mkstemp(prefix="rze-launch-on-output-", suffix=".js")
    os.write(fd, script.encode())
    os.close(fd)

    def call(obj, iface, method, args):
        return bus.call_sync("org.kde.KWin", obj, iface, method, args, None, Gio.DBusCallFlags.NONE, -1, None)

    try:
        script_id = call("/Scripting", "org.kde.kwin.Scripting", "loadScript",
                         GLib.Variant("(ss)", (path, script_name))).unpack()[0]
        call("/Scripting/Script%d" % script_id, "org.kde.kwin.Script", "run", None)
        GLib.timeout_add(POLL_MS, poll)
        loop.run()
    finally:
        try:
            call("/Scripting", "org.kde.kwin.Scripting", "unloadScript", GLib.Variant("(s)", (script_name,)))
            log.write("unloaded the KWin script")
        finally:
            Gio.bus_unown_name(owner)
            bus.unregister_object(registration)
            os.unlink(path)


def describe_outputs(outputs):
    return ", ".join("%s (%d,%d %dx%d)" % ((o["name"],) + tuple(o["geometry"])) for o in outputs)


def list_outputs():
    found = {}

    def on_report(report):
        found.update(report)
        return False

    deadline = time.monotonic() + READY_TIMEOUT_S
    run_kwin_script(os.getpid(), None, True, on_report, lambda: time.monotonic() < deadline, Log(None))
    if "detail" not in found:
        raise RuntimeError("KWin didn't answer")
    for output in found["detail"]:
        print("%s\t%d,%d %dx%d" % ((output["name"],) + tuple(output["geometry"])))


def watch(app_pid, output, timeout_s, ready_fd, log):
    """Tells app_pid (over ready_fd) once the script runs, then keeps it loaded until it's done with the program."""
    state = {"told": False, "ready": False, "main_at": None, "exited": False}
    started = time.monotonic()

    def tell(line):
        if not state["told"]:
            state["told"] = True
            os.write(ready_fd, (line + "\n").encode())
            os.close(ready_fd)

    def on_report(report):
        event = report["event"]
        if event == "ready":
            state["ready"] = True
            tell("ready")
        elif event == "error":
            detail = report["detail"]
            tell("error: %s; outputs: %s" % (detail["message"], describe_outputs(detail["outputs"])))
            return False
        elif event == "main":
            state["main_at"] = time.monotonic()
        return True

    def keep_going():
        # Before the handshake, app_pid is the launcher waiting on ready_fd; after it, the program
        try:
            os.kill(app_pid, 0)
        except ProcessLookupError:
            state["exited"] = True
            return False
        now = time.monotonic()
        if state["main_at"] is not None:
            return now - state["main_at"] < SETTLE_S
        return now - started < (timeout_s if state["ready"] else READY_TIMEOUT_S)

    try:
        run_kwin_script(app_pid, output, False, on_report, keep_going, log)
    except Exception as error:  # KWin missing, D-Bus down, ...: the launcher hears it and doesn't start the program
        log.write("failed: %s" % error)
        tell("error: couldn't run a KWin script (is this a KDE Plasma session?): %s" % error)
        return 1
    tell("error: KWin didn't run the script within %ds" % READY_TIMEOUT_S)
    if state["ready"] and state["main_at"] is None:
        log.write("the program exited before its main window appeared" if state["exited"]
                  else "timed out after %ds before the program's main window appeared" % timeout_s)
    return 0


def start_watcher(output, timeout_s, log_path):
    """Forks the watcher and returns its first line: "ready", or why the program shouldn't start."""
    app_pid = os.getpid()
    read_fd, write_fd = os.pipe()
    child = os.fork()
    if child == 0:
        os.close(read_fd)
        # Twice, so the watcher isn't left a zombie under the program, which won't reap it. In a session of its own, so
        # ending the program's process group (as the driver does) still leaves the watcher to unload the script.
        if os.fork() != 0:
            os._exit(0)
        os.setsid()
        # Nothing of the program's: a harness reads the program's stdout, and waits for it to close
        null = os.open(os.devnull, os.O_RDWR)
        for stream in (0, 1, 2):
            os.dup2(null, stream)
        status = 1
        try:
            status = watch(app_pid, output, timeout_s, write_fd, Log(log_path))
        finally:
            os._exit(status)
    os.close(write_fd)
    os.waitpid(child, 0)
    answer = b""
    while not answer.endswith(b"\n"):
        chunk = os.read(read_fd, 4096)
        if not chunk:
            break
        answer += chunk
    os.close(read_fd)
    return answer.decode().strip() or "error: the watcher died before KWin answered"


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--list", action="store_true", help="print KWin's outputs: name, then x,y widthxheight")
    parser.add_argument("--output", help="the output to open on, by KWin's name for it (e.g. DP-2)")
    parser.add_argument("--timeout", type=float, default=120,
                        help="seconds to wait for the main window before unloading the script")
    parser.add_argument("--log", help="a file for what the watcher saw (it has no stdout of its own)")
    parser.add_argument("command", nargs=argparse.REMAINDER, help="the program and its arguments, after --")
    args = parser.parse_args()

    if args.list:
        list_outputs()
        return 0
    command = args.command[1:] if args.command[:1] == ["--"] else args.command
    if not args.output or not command:
        parser.error("--output and a program are needed (or --list)")
    answer = start_watcher(args.output, args.timeout, args.log)
    if answer != "ready":
        print("LaunchOnOutput: %s" % answer, file=sys.stderr)
        return 1
    os.execvp(command[0], command)


if __name__ == "__main__":
    sys.exit(main())
