"""Archive one team's closed issues, by the archiving rule of the linear skill.

An issue is archived when it is Done or Canceled (any completed or canceled
state) and either has no parent, or its parent is closed too. A parent takes
all its children with it, whatever their state. A closed child of an open
parent stays until its parent closes.

    python3 archive_closed.py --team RZG --dry-run   # list what would be archived
    python3 archive_closed.py --team RZG             # archive it

The team key comes from the repository's .claude/workflow/repo.md. A parent takes
its children with it even when a child belongs to another team.

The API key is read from ~/.config/linear-api-key and never printed.
"""

from __future__ import annotations

import json
import sys
import urllib.error
import urllib.request
from pathlib import Path

API = "https://api.linear.app/graphql"
KEY_FILE = Path.home() / ".config" / "linear-api-key"

ISSUES = """query($after: String, $team: String!) {
  issues(first: 100, after: $after, filter: { team: { key: { eq: $team } } }) { nodes {
  id identifier title archivedAt state { name type } team { key }
  parent { identifier state { type } }
  children { nodes { id identifier archivedAt state { type } } } }
  pageInfo { hasNextPage endCursor } } }"""
ARCHIVE = "mutation($id: String!) { issueArchive(id: $id) { success } }"


def gql(query: str, variables: dict | None = None) -> dict:
    body = json.dumps({"query": query, "variables": variables or {}}).encode()
    request = urllib.request.Request(API, data=body, headers={
        "Content-Type": "application/json",
        "Authorization": KEY_FILE.read_text().strip(),
    })
    try:
        with urllib.request.urlopen(request) as response:
            data = json.load(response)
    except urllib.error.HTTPError as exc:  # Linear explains a 400 in the body
        data = json.load(exc)
    if "errors" in data:
        raise SystemExit(f"Linear: {json.dumps(data['errors'])[:500]}")
    return data["data"]


def closed(state: dict) -> bool:
    return state["type"] in ("completed", "canceled")


def to_archive(team: str) -> list[dict]:
    """The issues to archive, children before their parents."""
    rows, after = [], None
    while True:
        page = gql(ISSUES, {"after": after, "team": team})["issues"]
        rows += page["nodes"]
        if not page["pageInfo"]["hasNextPage"]:
            break
        after = page["pageInfo"]["endCursor"]
    by_id = {r["identifier"]: r for r in rows}
    chosen: dict[str, dict] = {}
    for row in rows:
        parent = row["parent"]
        if row["archivedAt"] or not closed(row["state"]):
            continue
        if parent is not None and not closed(parent["state"]):
            continue  # waits for its parent
        for child in row["children"]["nodes"]:  # a parent takes all its children
            if not child["archivedAt"]:
                chosen[child["identifier"]] = by_id.get(child["identifier"], child)
        chosen.setdefault(row["identifier"], row)
    parents = {r["identifier"] for r in chosen.values() if r.get("children", {}).get("nodes")}
    return sorted(chosen.values(), key=lambda r: r["identifier"] in parents)  # children first


def main() -> None:
    args = sys.argv[1:]
    dry = "--dry-run" in args
    if "--team" not in args or args.index("--team") + 1 >= len(args):
        raise SystemExit("usage: archive_closed.py --team <KEY> [--dry-run]")
    issues = to_archive(args[args.index("--team") + 1])
    for issue in issues:
        state = issue["state"]["type"]
        if dry:
            print(f"would archive {issue['identifier']} ({state})")
            continue
        ok = gql(ARCHIVE, {"id": issue["id"]})["issueArchive"]["success"]
        print(f"{'archived' if ok else 'FAILED'} {issue['identifier']} ({state})")
    print(f"{len(issues)} issue(s){' to archive' if dry else ''}.")


if __name__ == "__main__":
    main()
