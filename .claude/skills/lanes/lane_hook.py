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
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from lane_state import answered  # noqa: E402

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


tp = data.get("transcript_path") or previous.get("transcript", "")
state_name = "attention"
if event == "done":
    state_name, what = "done", None  # silent: the Stop that ends this turn reports it
elif event == "start":
    state_name, what = "working", None  # silent: the tab turns orange as soon as the lane starts
elif event == "stop":
    text = " ".join(last_text().split())[-200:] or "(no text)"
    if previous.get("state") == "done" and not answered(tp, previous.get("offset", 0)):
        state_name, what = "done", "free: " + text  # the Stop that ends the done command's turn
    else:
        what = "waiting for you: " + text
else:
    msg = data.get("message", "")
    if "permission" not in msg.lower():
        sys.exit(0)  # idle reminders repeat the stop event
    what = "needs permission: " + msg

state = {"session": os.environ.get("KONSOLE_DBUS_SESSION", "") or previous.get("session", ""),
         "transcript": tp, "offset": size(tp), "state": state_name, "event": event}
(root / "state").mkdir(parents=True, exist_ok=True)
state_file.write_text(json.dumps(state))
if what:
    with (root / "attention.log").open("a") as f:
        f.write(f"{time.strftime('%H:%M')} {lane} {what}\n")
