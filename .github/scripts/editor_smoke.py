#!/usr/bin/env python3
"""Editor smoke test, for CI: starts EditorHarness on a scene, checks it loads and keeps rendering, takes a shot,
closes it the way the close button does, and checks it exits cleanly.

    python3 .github/scripts/editor_smoke.py --config debug --out smoke-out

Fails (exit code 1) if the harness doesn't get ready, the scene doesn't load or lacks an expected object, frames stop,
the shot fails, or the Editor doesn't exit with code 0. Errors and warnings the Editor logs are listed, and annotated
on GitHub, but don't fail it. Debug runs with -gfxliveobjects, so graphics objects left alive at shutdown are listed
too, as their count. Writes the Editor's output (app.log) and the shot (shot.png) to --out.

Needs a display: on Linux CI, run it under xvfb-run with SDL_VIDEO_DRIVER=x11. RZE/Config/imgui.ini, which every
Editor run rewrites, is put back afterwards, so running it locally keeps your layout. Standard library only."""
import argparse
import json
import os
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import threading
import time

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
RZE_DIR = os.path.join(REPO_ROOT, "RZE")
IMGUI_INI = os.path.join(RZE_DIR, "Config", "imgui.ini")
# HarnessChannel::k_replyPrefix: the harness's answers, among the Editor's log lines on stdout
REPLY_PREFIX = "RZEHARNESS "
ON_GITHUB = os.environ.get("GITHUB_ACTIONS") == "true"
# -gfxliveobjects' report: a line for each D3D11 object still alive at shutdown, each at its own address. They stay in
# app.log; the count the report ends with ("N D3D11 objects outlived shutdown ...") is what gets listed.
LIVE_OBJECT_LINE = re.compile(r"^\[Warning\] D3D11 \(\d+\): \s*Live ")


class SmokeFailure(Exception):
    pass


def log(message):
    print(message, flush=True)


def annotate(level, message):
    """A line in the job's summary on GitHub; just the message elsewhere."""
    if ON_GITHUB:
        # Workflow commands end at a newline, and % starts an escape
        print("::%s::%s" % (level, message.replace("%", "%25").replace("\r", "").replace("\n", "%0A")), flush=True)


class Harness:
    """EditorHarness, with its stdin and stdout: commands in, answers and log lines out."""

    def __init__(self, config, scene, app_log_path):
        build_dir = os.path.join(RZE_DIR, "_build", config)
        exe = os.path.join(build_dir, "EditorHarness.exe" if os.name == "nt" else "EditorHarness")
        if not os.path.isfile(exe):
            raise SmokeFailure("No %s: build the %s configuration first" % (exe, config))
        self.app_log = open(app_log_path, "w", encoding="utf-8")
        self.errors = []
        self.warnings = []
        self.ready = False
        self.answers = {}
        self.next_id = 0
        self.changed = threading.Condition()
        command = [exe, "-scene", scene]
        # Debug validates already, so the report of the graphics objects still alive at shutdown changes nothing else
        if config == "debug":
            command.append("-gfxliveobjects")
        self.process = subprocess.Popen(command, cwd=build_dir, stdin=subprocess.PIPE,
                                        stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        self.reader = threading.Thread(target=self._read_output, daemon=True)
        self.reader.start()

    def _read_output(self):
        for raw in self.process.stdout:
            line = raw.decode(errors="replace").rstrip("\r\n")
            if line.startswith(REPLY_PREFIX):
                answer = json.loads(line[len(REPLY_PREFIX):])
                with self.changed:
                    if answer.get("event") == "ready":
                        self.ready = True
                    elif "id" in answer:
                        self.answers[answer["id"]] = answer
                    self.changed.notify_all()
                continue
            self.app_log.write(line + "\n")
            self.app_log.flush()
            # Warnings are listed once each at the end, as a driver can repeat one every frame
            if line.startswith("[Error]"):
                self.errors.append(line)
            elif line.startswith("[Warning]"):
                if not LIVE_OBJECT_LINE.match(line):
                    self.warnings.append(line)
                continue
            if line.strip() and not line.startswith("[Debug]"):
                log("  | " + line)
        with self.changed:
            self.changed.notify_all()

    def _wait(self, done, timeout, what):
        deadline = time.monotonic() + timeout
        with self.changed:
            while not done():
                if self.process.poll() is not None:
                    raise SmokeFailure("The Editor exited (%s) while waiting for %s" % (describe_exit(self.process.returncode), what))
                if time.monotonic() > deadline:
                    raise SmokeFailure("No %s after %d s" % (what, timeout))
                self.changed.wait(0.2)

    def wait_ready(self, timeout):
        self._wait(lambda: self.ready, timeout, "ready event")

    def request(self, command, timeout=60, **fields):
        """Sends one command and returns its answer; SmokeFailure if there's none, or it says it failed."""
        self.next_id += 1
        request_id = self.next_id
        try:
            self.process.stdin.write((json.dumps(dict(fields, cmd=command, id=request_id)) + "\n").encode())
            self.process.stdin.flush()
        except OSError:
            code = self.process.poll()
            raise SmokeFailure("The Editor stopped reading commands (%s)" % ("still running" if code is None else describe_exit(code)))
        self._wait(lambda: request_id in self.answers, timeout, "answer to " + command)
        with self.changed:
            answer = self.answers.pop(request_id)
        if not answer.get("ok"):
            raise SmokeFailure("%s failed: %s" % (command, answer.get("error")))
        return answer

    def wait_exit(self, timeout):
        try:
            code = self.process.wait(timeout)
        except subprocess.TimeoutExpired:
            raise SmokeFailure("The Editor was still running %d s after quit" % timeout)
        # The rest of its output, so the log is whole
        self.reader.join(10)
        return code

    def kill(self):
        if self.process.poll() is None:
            self.process.kill()
            self.process.wait(10)
        self.reader.join(10)
        self.app_log.close()


def png_size(path):
    with open(path, "rb") as png:
        header = png.read(24)
    if header[:8] != b"\x89PNG\r\n\x1a\n":
        raise SmokeFailure("%s isn't a PNG" % path)
    return struct.unpack(">II", header[16:24])


def distinct_with_counts(lines):
    """Each distinct line, in the order it first came, with its count."""
    counts = {}
    for line in lines:
        counts[line] = counts.get(line, 0) + 1
    return list(counts.items())


def describe_exit(code):
    # A negative code is the signal that ended it (POSIX); Windows gives an NTSTATUS such as 0xC0000005
    if code < 0:
        return "killed by signal %d" % -code
    return "code %d (0x%08X)" % (code, code & 0xFFFFFFFF)


def run(args, harness):
    started = time.monotonic()
    elapsed = lambda: "%.1f s" % (time.monotonic() - started)

    harness.wait_ready(args.timeout)
    log("ready after %s" % elapsed())

    deadline = time.monotonic() + args.timeout
    while True:
        state = harness.request("state")
        if not state["loading"]:
            break
        if time.monotonic() > deadline:
            raise SmokeFailure("%s was still loading after %d s" % (args.scene, args.timeout))
        time.sleep(0.5)
    names = [entry["name"] for entry in state["objects"]]
    log("loaded after %s: %s" % (elapsed(), ", ".join(names)))
    if state["scene"] is None or not state["scene"].endswith(args.scene):
        raise SmokeFailure("The Editor has %s open, not %s" % (state["scene"], args.scene))
    missing = [name for name in args.expect if name not in names]
    if missing:
        raise SmokeFailure("%s has no %s" % (args.scene, ", ".join(missing)))
    if state["modified"]:
        raise SmokeFailure("The scene shows unsaved changes straight after loading")

    # Frames keep coming: the render thread and the main loop are both alive. Then a few more, so meshes the scene
    # streams in after loading have had time to arrive before the shot
    info = harness.request("info")
    first_frame = info["frame"]
    deadline = time.monotonic() + args.timeout
    while harness.request("info")["frame"] < first_frame + args.frames:
        if time.monotonic() > deadline:
            raise SmokeFailure("Fewer than %d frames in %d s after loading" % (args.frames, args.timeout))
        time.sleep(0.5)
    log("%d frames after %s (%dx%d client)" % (args.frames, elapsed(), info["width"], info["height"]))

    shot_path = os.path.abspath(os.path.join(args.out, "shot.png"))
    harness.request("shot", path=shot_path)
    size = png_size(shot_path)
    if size != (info["width"], info["height"]):
        raise SmokeFailure("The shot is %dx%d, the client area %dx%d" % (size + (info["width"], info["height"])))
    log("shot %s (%dx%d)" % (shot_path, size[0], size[1]))

    # As the window's close button does; with no unsaved changes, nothing asks first
    harness.request("quit")
    code = harness.wait_exit(args.exit_timeout)
    log("exited after %s with %s" % (elapsed(), describe_exit(code)))
    if code != 0:
        raise SmokeFailure("The Editor exited with %s" % describe_exit(code))


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--config", required=True, choices=["debug", "development", "retail"])
    parser.add_argument("--scene", default="Sponza.scene", help="in RZE/Assets/Scenes")
    parser.add_argument("--expect", default="Camera,DirectionalLight,Sponza,Nyra,Nanosuit,NeckMechWalker,Tank",
                        help="comma-separated objects the scene must have")
    parser.add_argument("--frames", type=int, default=10, help="frames to wait for after loading, before the shot")
    parser.add_argument("--timeout", type=int, default=120, help="seconds for each of ready, loading and frames")
    parser.add_argument("--exit-timeout", type=int, default=120, help="seconds for the Editor to exit after quit")
    parser.add_argument("--out", default="smoke-out", help="folder for app.log and shot.png")
    args = parser.parse_args()
    args.expect = [name for name in args.expect.split(",") if name]
    os.makedirs(args.out, exist_ok=True)

    backup_dir = tempfile.mkdtemp(prefix="rze-smoke-")
    backup = os.path.join(backup_dir, "imgui.ini")
    if os.path.exists(IMGUI_INI):
        shutil.copyfile(IMGUI_INI, backup)

    log("EditorHarness (%s) on %s" % (args.config, args.scene))
    harness = None
    failure = None
    try:
        harness = Harness(args.config, args.scene, os.path.join(args.out, "app.log"))
        run(args, harness)
    except SmokeFailure as error:
        failure = str(error)
    finally:
        if harness is not None:
            harness.kill()
        # A copy, so the file gets a new timestamp
        if os.path.exists(backup):
            shutil.copyfile(backup, IMGUI_INI)
        shutil.rmtree(backup_dir, ignore_errors=True)

    if harness is not None:
        log("logged %d errors, %d warnings" % (len(harness.errors), len(harness.warnings)))
        # Each distinct line once, with how often it came: a driver can repeat one every frame. As warnings on GitHub,
        # errors too: they're reported, but don't fail the job
        for line, count in distinct_with_counts(harness.errors + harness.warnings):
            text = line if count == 1 else "%s (x%d)" % (line, count)
            log("  " + text)
            annotate("warning", "Editor (%s) logged: %s" % (args.config, text))

    if failure is not None:
        annotate("error", "Editor smoke test (%s): %s" % (args.config, failure))
        log("FAIL: " + failure)
        return 1
    log("PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
