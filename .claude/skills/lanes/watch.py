"""The coordinator's lane watch (lanes skill, step 2). Run it in the background, output discarded:
    python3 -u watch.py <dir> > /dev/null 2>&1
Prints each new line of <dir>/attention.log (one event per lane that needs the user; the
coordinator doesn't relay them, step 3), and colours each lane's Konsole tab: red while it needs
the user, orange while it's working (its session has just started, or the user answered: a new
answer in the lane's transcript after the point the hook recorded), blue while it has stopped but
is waiting on one of its own shell commands (a background task), not the user, and green once it
has landed its work and is free for more (the lane's done command).

While a lane runs its tests, its tab turns blue (unless a permission prompt is waiting) and its
colour strip becomes a progress bar (Konsole's OSC 9;4 progress, written to the lane's terminal).
The bar shows tests finished out of the total, from <dir>/progress/<lane>.json ({"done", "total",
"run", "runs"}, written by the repository's test runner); without that file it sweeps. With the
file, the tab's label also gets " · <run>/<runs> · <percent>%" after its own title format, which is
put back exactly when the run ends. A test run is a process
under the lane's Claude whose command line matches the "Test runs" row of
<main>/.claude/workflow/repo.md (a regular expression). Without that row, there is no bar.
"""
import json
import os
import re
import subprocess
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from lane_state import answered  # noqa: E402

ROOT = Path(sys.argv[1])
LOG, STATE, PROGRESS = ROOT / "attention.log", ROOT / "state", ROOT / "progress"
COLOURS = {"attention": "#c0392b", "working": "#d68910", "shell": "#2f6fd0", "done": "#3b8f5a"}
SERVICE = os.environ["KONSOLE_DBUS_SERVICE"]
REPO_MD = Path(__file__).resolve().parents[3] / ".claude" / "workflow" / "repo.md"
SWEEP_STEP = 10  # percent per second: one sweep every ten seconds


def test_pattern():
    try:
        m = re.search(r"^\|\s*Test runs\s*\|[^`\n]*`([^`]+)`", REPO_MD.read_text(), re.M)
        return re.compile(m.group(1)) if m else None
    except (OSError, re.error):
        return None


def colour(session, state):
    if session:
        subprocess.run(["qdbus-qt6", SERVICE, session, "setTabColor", COLOURS[state]],
                       capture_output=True)


def process_table():
    """Each process's children and command line, read once per pass from /proc."""
    children, cmdlines = {}, {}
    for entry in os.scandir("/proc"):
        if not entry.name.isdigit():
            continue
        try:
            with open(f"/proc/{entry.name}/stat", "rb") as f:
                ppid = int(f.read().rsplit(b")", 1)[1].split()[1])
            with open(f"/proc/{entry.name}/cmdline", "rb") as f:
                cmdlines[entry.name] = f.read().replace(b"\0", b" ").decode(errors="replace").strip()
        except (OSError, ValueError, IndexError):
            continue
        children.setdefault(str(ppid), []).append(entry.name)
    return children, cmdlines


def descendants(children, pid):
    out, todo = [], list(children.get(pid, []))
    while todo:
        p = todo.pop()
        out.append(p)
        todo.extend(children.get(p, []))
    return out


def lane_processes(session):
    """The tab's shell pid and the pids of the lane's claude processes under it."""
    shell = subprocess.run(["qdbus-qt6", SERVICE, session, "processId"],
                           capture_output=True, text=True).stdout.strip()
    claude = subprocess.run(["pgrep", "-P", shell, "-x", "claude"],
                            capture_output=True, text=True).stdout.split() if shell.isdigit() else []
    return shell, claude


def shell_running(table, claude):
    """True while the lane's Claude has a shell command running (Claude's Bash runs via shell-snapshots)."""
    children, cmdlines = table
    return any("shell-snapshots" in cmdlines.get(c, "") for pid in claude for c in children.get(pid, []))


def testing(table, claude, pattern):
    children, cmdlines = table
    return pattern is not None and any(pattern.search(cmdlines.get(p, ""))
                                       for pid in claude for p in descendants(children, pid))


SUFFIX = re.compile(r" · \d+/\d+ · \d+%$")  # what label() adds; stripped if the watch restarts mid-run


def counted(lane):
    """(percent 0-100, run, runs) from the lane's progress file; None without one."""
    try:
        d = json.loads((PROGRESS / f"{lane}.json").read_text())
        if d["total"] <= 0:
            return None
        return max(0, min(100, int(100 * d["done"] / d["total"]))), int(d["run"]), int(d["runs"])
    except (OSError, ValueError, KeyError, TypeError):
        return None


def tab_format(session, context):
    return subprocess.run(["qdbus-qt6", SERVICE, session, "tabTitleFormat", str(context)],
                          capture_output=True, text=True).stdout.rstrip("\n")


def set_tab_format(session, formats, suffix=""):
    """Set both title formats. Konsole shows the change at its next refresh, about 2 s after the
    tab's last output. Never use D-Bus setTitle 1 to hurry it: that also overwrites both formats
    with the literal text, which would lose `%w`."""
    for context, fmt in enumerate(formats):
        subprocess.run(["qdbus-qt6", SERVICE, session, "setTabTitleFormat", str(context), fmt + suffix],
                       capture_output=True)


def progress(shell, value):
    """Write Konsole's progress sequence to the lane's terminal: 0-100, or None to hide it."""
    try:
        tty = os.readlink(f"/proc/{shell}/fd/0")
        if not tty.startswith("/dev/pts/"):
            return
        seq = "\x1b]9;4;0\x07" if value is None else f"\x1b]9;4;1;{value}\x07"
        fd = os.open(tty, os.O_WRONLY | os.O_NOCTTY)
        try:
            os.write(fd, seq.encode())
        finally:
            os.close(fd)
    except OSError:
        pass


STATE.mkdir(parents=True, exist_ok=True)
LOG.touch()
pos = LOG.stat().st_size
shown = {}
bars = {}  # lane state file name -> the lane's shell pid while its bar is up
labels = {}  # lane state file name -> (session, its own two title formats, the suffix now shown)
tick = 0
while True:
    if LOG.stat().st_size > pos:
        with LOG.open() as f:
            f.seek(pos)
            for line in f:
                print(line.rstrip(), flush=True)
            pos = f.tell()
    pattern = test_pattern()
    table = process_table()
    for p in STATE.glob("*.json"):
        try:
            s = json.loads(p.read_text())
        except ValueError:
            continue
        if s["state"] in ("attention", "done") and answered(s["transcript"], s["offset"]):
            s["state"] = "working"
            p.write_text(json.dumps(s))
        shell, claude = lane_processes(s["session"]) if s["session"] else ("", [])
        tests = bool(shell) and testing(table, claude, pattern)
        permission = s["state"] == "attention" and s.get("event") == "notify"
        shown_as = s["state"]
        if tests and not permission:
            shown_as = "shell"  # running its tests: blue, and the bar below shows how far
        elif shown_as == "attention" and not permission and shell_running(table, claude):
            shown_as = "shell"  # stopped, but waiting on its own shell command, not the user
        if shown.get(p.name) != (s["session"], shown_as):
            colour(s["session"], shown_as)
            shown[p.name] = (s["session"], shown_as)
        count = counted(p.stem) if tests else None
        if tests:
            value = count[0] if count else min((tick * SWEEP_STEP) % 110, 100)  # no count: sweep
            progress(shell, value)
            bars[p.name] = shell
        elif p.name in bars:
            progress(bars.pop(p.name), None)
        if count and s["session"]:
            suffix = f" · {count[1]}/{count[2]} · {count[0]}%"
            if p.name not in labels:
                own = [SUFFIX.sub("", tab_format(s["session"], c)) for c in (0, 1)]
                labels[p.name] = (s["session"], own, None)
            session, own, shown_suffix = labels[p.name]
            if shown_suffix != suffix:
                set_tab_format(session, own, suffix)
                labels[p.name] = (session, own, suffix)
        elif p.name in labels:
            session, own, _ = labels.pop(p.name)
            set_tab_format(session, own)  # the run ended: the tab's own title format, exactly
    for name in [n for n in bars if not (STATE / n).exists()]:
        progress(bars.pop(name), None)  # the lane was cleaned up mid-run
    for name in [n for n in labels if not (STATE / n).exists()]:
        session, own, _ = labels.pop(name)
        set_tab_format(session, own)
    tick += 1
    time.sleep(1)
