"""Compare and copy the shared workflow skills between this repository and its peer.

The shared set is SHARED below, under .claude/skills/. The per-repository files in
.claude/workflow/ are never touched. The peer comes from this repository's
.claude/workflow/repo.md ("Peer repository" row), or --peer PATH.

    python3 sync.py                    # status: same / differs / only here / only there
    python3 sync.py diff [FILE ...]    # unified diffs, here (a) against the peer (b)
    python3 sync.py push FILE ... | all   # copy here -> peer
    python3 sync.py pull FILE ... | all   # copy peer -> here

FILE is a path under .claude/skills/, e.g. lanes/SKILL.md. "all" means every file
that differs or exists on one side only. A copy is refused when the file it would
overwrite has uncommitted changes. Nothing is committed: commit each side yourself.
"""

from __future__ import annotations

import difflib
import re
import shutil
import subprocess
import sys
from pathlib import Path

SHARED = ("lanes", "work-graph", "linear", "sync-workflow")
HERE = Path(__file__).resolve().parents[3]


def skills(root: Path) -> Path:
    return root / ".claude" / "skills"


def peer_from_repo_md() -> Path:
    repo_md = HERE / ".claude" / "workflow" / "repo.md"
    match = re.search(r"^\|\s*Peer repository\s*\|\s*`([^`]+)`", repo_md.read_text(), re.M)
    if not match:
        raise SystemExit(f"no 'Peer repository' row in {repo_md}; pass --peer PATH")
    return Path(match.group(1))


def files(root: Path) -> set[str]:
    base = skills(root)
    found = set()
    for name in SHARED:
        for path in (base / name).rglob("*"):
            if path.is_file() and "__pycache__" not in path.parts:
                found.add(path.relative_to(base).as_posix())
    return found


def status(peer: Path) -> dict[str, str]:
    here, there = files(HERE), files(peer)
    result = {}
    for rel in sorted(here | there):
        if rel not in there:
            result[rel] = "only here"
        elif rel not in here:
            result[rel] = "only there"
        elif (skills(HERE) / rel).read_bytes() != (skills(peer) / rel).read_bytes():
            result[rel] = "differs"
        else:
            result[rel] = "same"
    return result


def dirty(root: Path, rel: str) -> bool:
    path = skills(root) / rel
    out = subprocess.run(["git", "-C", str(root), "status", "--porcelain", "--", str(path)],
                         capture_output=True, text=True, check=True).stdout
    return bool(out.strip())


def pick(peer: Path, names: list[str]) -> list[str]:
    states = status(peer)
    if names == ["all"]:
        return [rel for rel, state in states.items() if state != "same"]
    unknown = [n for n in names if n not in states]
    if unknown:
        raise SystemExit(f"not a shared file: {', '.join(unknown)}")
    return names


def copy(src_root: Path, dst_root: Path, names: list[str]) -> None:
    refused = [rel for rel in names if (skills(dst_root) / rel).exists() and dirty(dst_root, rel)]
    if refused:
        raise SystemExit(f"uncommitted changes in {dst_root}, nothing copied: {', '.join(refused)}")
    for rel in names:
        src, dst = skills(src_root) / rel, skills(dst_root) / rel
        if not src.exists():
            print(f"skipped {rel}: missing in {src_root} (delete it in {dst_root} by hand if it should go)")
            continue
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src, dst)
        print(f"copied {rel} -> {dst_root}")


def main() -> None:
    args = sys.argv[1:]
    peer = None
    if "--peer" in args:
        i = args.index("--peer")
        peer = Path(args[i + 1])
        del args[i:i + 2]
    peer = (peer or peer_from_repo_md()).resolve()
    command, names = (args[0], args[1:]) if args else ("status", [])

    if command == "status":
        print(f"here:  {HERE}\nthere: {peer}")
        for rel, state in status(peer).items():
            print(f"  {state:<10} {rel}")
    elif command == "diff":
        for rel in pick(peer, names or ["all"]):
            a, b = skills(HERE) / rel, skills(peer) / rel
            lines = [p.read_text().splitlines(keepends=True) if p.exists() else [] for p in (a, b)]
            sys.stdout.writelines(difflib.unified_diff(*lines, f"a/{rel}", f"b/{rel}"))
    elif command in ("push", "pull") and names:
        src, dst = (HERE, peer) if command == "push" else (peer, HERE)
        copy(src, dst, pick(peer, names))
    else:
        raise SystemExit(__doc__)


if __name__ == "__main__":
    main()
