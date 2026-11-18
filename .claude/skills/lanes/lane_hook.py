"""A lane's attention hook (lanes skill, step 2).

Run by the lane's Claude session from its --settings file:
    python3 lane_hook.py <dir> <lane> stop      # Stop: the lane finished its turn
    python3 lane_hook.py <dir> <lane> notify    # Notification: only permission prompts count
It appends one line to <dir>/attention.log and records <dir>/state/<lane>.json, which
watch.py uses to colour the lane's tab and to notice when the user has answered.
"""
import json
import os
import sys
import time
from pathlib import Path

root, lane, event = Path(sys.argv[1]), sys.argv[2], sys.argv[3]
try:
    data = json.load(sys.stdin)
except ValueError:
    data = {}


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


if event == "stop":
    what = "waiting for you: " + (" ".join(last_text().split())[-200:] or "(no text)")
else:
    msg = data.get("message", "")
    if "permission" not in msg.lower():
        sys.exit(0)  # idle reminders repeat the stop event
    what = "needs permission: " + msg

tp = data.get("transcript_path", "")
state = {"session": os.environ.get("KONSOLE_DBUS_SESSION", ""), "transcript": tp,
         "offset": os.path.getsize(tp) if tp and os.path.exists(tp) else 0, "state": "attention", "event": event}
(root / "state").mkdir(parents=True, exist_ok=True)
(root / "state" / f"{lane}.json").write_text(json.dumps(state))
with (root / "attention.log").open("a") as f:
    f.write(f"{time.strftime('%H:%M')} {lane} {what}\n")
