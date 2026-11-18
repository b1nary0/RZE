"""Used by watch.py (lanes skill, "Attention highlights") to notice when the user has answered."""
import json


def is_answer(record):
    """A user record that carries the user's words, not only tool results.

    Tool results are stored as "user" records too; a lane's own commands (the done
    command, for one) must not count as the user answering.
    """
    if record.get("type") != "user" or record.get("isMeta"):
        return False
    content = record.get("message", {}).get("content")
    if isinstance(content, str):
        return bool(content.strip())
    return any(isinstance(p, dict) and p.get("type") != "tool_result" for p in content or [])


def answered(transcript, offset):
    """True when the transcript has an answer from the user after byte offset."""
    try:
        with open(transcript, "rb") as f:
            f.seek(offset)
            for line in f:
                try:
                    if is_answer(json.loads(line)):
                        return True
                except ValueError:
                    pass
    except OSError:
        pass
    return False
