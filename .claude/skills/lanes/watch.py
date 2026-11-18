"""The coordinator's lane watch (lanes skill, step 3). Run it as a Monitor:
    python3 -u watch.py <dir>
Prints each new line of <dir>/attention.log (one event per lane that needs the user), and
colours each lane's Konsole tab: red while it needs the user, orange while it's working (the user
answered: a new answer in the lane's transcript after the point the hook recorded), blue while it
has stopped but is waiting on one of its own shell commands (a background task), not the user, and
green once it has landed its work and is free for more (the lane's done command).
"""
import json
import os
import subprocess
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from lane_state import answered  # noqa: E402

ROOT = Path(sys.argv[1])
LOG, STATE = ROOT / "attention.log", ROOT / "state"
COLOURS = {"attention": "#c0392b", "working": "#d68910", "shell": "#2f6fd0", "done": "#3b8f5a"}
SERVICE = os.environ["KONSOLE_DBUS_SERVICE"]


def colour(session, state):
    if session:
        subprocess.run(["qdbus-qt6", SERVICE, session, "setTabColor", COLOURS[state]],
                       capture_output=True)


def shell_running(session):
    """True while the lane's Claude has a shell command running (Claude's Bash runs via shell-snapshots)."""
    try:
        shell = subprocess.run(["qdbus-qt6", SERVICE, session, "processId"],
                               capture_output=True, text=True).stdout.strip()
        claude = subprocess.run(["pgrep", "-P", shell, "-x", "claude"],
                                capture_output=True, text=True).stdout.split()
        return any(subprocess.run(["pgrep", "-P", pid, "-f", "shell-snapshots"],
                                  capture_output=True).returncode == 0 for pid in claude)
    except (OSError, ValueError):
        return False


STATE.mkdir(parents=True, exist_ok=True)
LOG.touch()
pos = LOG.stat().st_size
shown = {}
while True:
    if LOG.stat().st_size > pos:
        with LOG.open() as f:
            f.seek(pos)
            for line in f:
                print(line.rstrip(), flush=True)
            pos = f.tell()
    for p in STATE.glob("*.json"):
        try:
            s = json.loads(p.read_text())
        except ValueError:
            continue
        if s["state"] in ("attention", "done") and answered(s["transcript"], s["offset"]):
            s["state"] = "working"
            p.write_text(json.dumps(s))
        shown_as = s["state"]
        if shown_as == "attention" and s.get("event") != "notify" and shell_running(s["session"]):
            shown_as = "shell"  # stopped, but waiting on its own shell command, not the user
        if shown.get(p.name) != (s["session"], shown_as):
            colour(s["session"], shown_as)
            shown[p.name] = (s["session"], shown_as)
    time.sleep(1)
