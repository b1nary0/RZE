"""The coordinator's lane watch (lanes skill, step 3). Run it as a Monitor:
    python3 -u watch.py <dir>
Prints each new line of <dir>/attention.log (one event per lane that needs the user), and
colours each lane's Konsole tab: red while it needs the user, green once they have answered
(a new "user" record in the lane's transcript after the point the hook recorded), and blue while
it has stopped but is waiting on one of its own shell commands (a background task), not the user.
"""
import json
import os
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(sys.argv[1])
LOG, STATE = ROOT / "attention.log", ROOT / "state"
COLOURS = {"attention": "#c0392b", "working": "#3b8f5a", "shell": "#2f6fd0"}
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


def answered(transcript, offset):
    try:
        with open(transcript, "rb") as f:
            f.seek(offset)
            for line in f:
                try:
                    if json.loads(line).get("type") == "user":
                        return True
                except ValueError:
                    pass
    except OSError:
        pass
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
        if s["state"] == "attention" and answered(s["transcript"], s["offset"]):
            s["state"] = "working"
            p.write_text(json.dumps(s))
        shown_as = s["state"]
        if shown_as == "attention" and s.get("event") != "notify" and shell_running(s["session"]):
            shown_as = "shell"  # stopped, but waiting on its own shell command, not the user
        if shown.get(p.name) != (s["session"], shown_as):
            colour(s["session"], shown_as)
            shown[p.name] = (s["session"], shown_as)
    time.sleep(1)
