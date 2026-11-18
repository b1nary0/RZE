"""A lane's attention hook (lanes skill, step 2 and "Attention highlights").

Run by the lane's Claude session from its --settings file:
    python3 lane_hook.py <dir> <lane> start     # SessionStart: the lane is working (silent)
    python3 lane_hook.py <dir> <lane> stop      # Stop: the lane finished its turn
    python3 lane_hook.py <dir> <lane> notify    # Notification: only permission prompts count
and by the lane itself, as the last landing step (the settings file's env sets both):
    python3 lane_hook.py "$LANE_DIR" "$LANE_NAME" done   # landed, free for more work
It appends one line to <dir>/attention.log and records <dir>/state/<lane>.json, which
watch.py uses to colour the lane's tab and to notice when the user has answered.
"""
import json
import os
import subprocess
import sys
import time
from pathlib import Path

root, lane, event = Path(sys.argv[1]), sys.argv[2], sys.argv[3]
state_file = root / "state" / f"{lane}.json"
data = {}
if event != "done":  # the hooks pass JSON on stdin; the lane's own done command doesn't
    try:
        data = json.load(sys.stdin)
    except ValueError:
        pass
try:
    previous = json.loads(state_file.read_text())
except (OSError, ValueError):
    previous = {}


def last_text():
    text = data.get("last_assistant_message") or ""
    if not text and data.get("transcript_path"):
        try:
            for line in Path(data["transcript_path"]).read_text().splitlines():
                try:
                    rec = json.loads(line)
                except ValueError:
                    continue
                if rec.get("type") == "assistant":
                    for part in rec.get("message", {}).get("content", []) or []:
                        if isinstance(part, dict) and part.get("type") == "text" and part["text"].strip():
                            text = part["text"]
        except OSError:
            pass
    return text


def size(path):
    return os.path.getsize(path) if path and os.path.exists(path) else 0


def branch(cwd):
    """The branch checked out in cwd; "" when detached or unknown."""
    try:
        return subprocess.run(["git", "-C", cwd, "branch", "--show-current"],
                              capture_output=True, text=True).stdout.strip()
    except OSError:
        return ""


tp = data.get("transcript_path") or previous.get("transcript", "")
# Landed: set by the done command, and kept until the lane starts another issue's branch (the
# small-items lane). The branch is read at the first Stop after done, from the session's own folder.
landed, landed_branch = previous.get("landed", False), previous.get("landed_branch")
if event == "done":
    landed, landed_branch = True, None
elif landed and event in ("start", "stop"):
    current = branch(data.get("cwd") or os.getcwd())
    if landed_branch is None:
        landed_branch = current
    if current not in (landed_branch, ""):
        landed, landed_branch = False, None  # on a new issue's branch: an ordinary lane again

state_name = "attention"
if event == "done":
    state_name, what = "done", None  # silent: the Stop that ends this turn reports it
elif event == "start":
    state_name, what = "working", None  # silent: the tab turns orange as soon as the lane starts
elif event == "stop":
    text = " ".join(last_text().split())[-200:] or "(no text)"
    if landed:
        state_name, what = "done", "free: " + text  # landed: every Stop is free, not waiting
    else:
        what = "waiting for you: " + text
else:
    msg = data.get("message", "")
    if "permission" not in msg.lower():
        sys.exit(0)  # idle reminders repeat the stop event
    what = "needs permission: " + msg

state = {"session": os.environ.get("KONSOLE_DBUS_SESSION", "") or previous.get("session", ""),
         "transcript": tp, "offset": size(tp), "state": state_name, "event": event,
         "landed": landed, "landed_branch": landed_branch}
(root / "state").mkdir(parents=True, exist_ok=True)
state_file.write_text(json.dumps(state))
if what:
    with (root / "attention.log").open("a") as f:
        f.write(f"{time.strftime('%H:%M')} {lane} {what}\n")
