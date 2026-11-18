---
name: sync-workflow
description: Compare and copy the shared workflow skills (lanes, work-graph, linear, sync-workflow) between this repository and its peers (each repository that uses them commits its own copy). Use when the user asks to sync the workflow skills, or after changing a shared skill, to offer carrying the change across.
---

# Sync the workflow skills

Each repository that uses the shared workflow skills commits its own copy, so each works without the others. **No repository names another:** `sync.py` finds the peers itself, so renaming or moving one repository never needs a change in the others. Only the skill folders are shared; each repository's `.claude/workflow/` (repo settings, Linear conventions, lane prompt) is its own and never synced.

**Keep the shared skills free of repository details.** Anything that differs between the repositories goes in `.claude/workflow/repo.md` (or `linear.md`, `lane-prompt.md`), and the skill refers to it.

## Steps

1. Status: `python3 .claude/skills/sync-workflow/sync.py` (from the repository root). Each shared file is `same`, `differs`, `only here` or `only there`. The peers are the folders beside this repository whose `repo.md` names that folder as its main checkout (lane worktrees don't count). With several peers, `status` shows each, and the other commands need `--peer PATH`.
2. For each file that isn't `same`, show the user `sync.py diff <file>` (here is `a/`, the peer is `b/`), summarised, and say which side looks newer and why.
3. **Ask** which way each file goes. Then `sync.py push <files>` (here → peer) or `sync.py pull <files>` (peer → here); `all` takes every file that isn't `same`.
   - A copy is refused when the file it would overwrite has uncommitted changes. Show the user; don't work around it.
   - A file deleted on one side isn't deleted on the other: say so, and delete it only on the user's word.
4. If a change needs a new per-repository setting, add it to the other repository's `.claude/workflow/` files too, with the user.
5. Nothing is committed. Offer one commit per repository that took changes, staging only the copied files (`Sync workflow skills: …`), and follow each repository's own commit rules (its `CLAUDE.md`; check `git status --short` first).
