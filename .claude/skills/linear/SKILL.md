---
name: linear
description: How this repository's work is tracked in Linear (workspace "rze"; the team is in .claude/workflow/repo.md): writing issues, the per-issue workflow and its states, epics, views and the API key, and archiving closed issues. Use whenever reading, creating, updating, closing or organising Linear issues, projects or views, and before working on an issue.
---

# Linear

This skill is shared between repositories (kept in step with `sync-workflow`). Two per-repository files go with it:

- `.claude/workflow/repo.md`: the team key, and whether the archive sweep is on.
- `.claude/workflow/linear.md`: the team's projects, how its issues are written, and its per-issue workflow. **Read it first**, before any Linear work.

Work only the team in `repo.md`, unless the user explicitly asks about another. Use the Linear MCP tools first. Use the API key only for what they can't do (archiving, views).

## Archive closed issues (every time, when `repo.md` turns the sweep on)

The rule, agreed with the user:

1. An issue that is **Done or Canceled** (any completed or canceled state) is archived:
   - if it has **no parent**, or
   - if its **parent is closed** too.
2. A closed issue whose parent is still open stays unarchived until the parent closes.
3. Archiving a parent **takes all its children with it**, whatever their state.

When to run it:
- **Right after you set an issue Done or Canceled.** Finish its edits first (the Result note, the commit hash).
- **At the start of any Linear work.** That catches issues the user closed in Linear itself.

How: `python3 .claude/skills/linear/archive_closed.py --team <TEAM>`, from the repository root (the exact command is in `repo.md`).
- It applies the rule to that team's issues only, archives children before their parents, and prints what it archived.
- `--dry-run` lists what it would archive without changing anything.
- The user has approved these archive writes in advance. Run it without asking, and say in one line what was archived.
- Archived issues stay searchable and can be restored, and links to them keep working. The MCP has no archive tool, which is why this uses the API.

## Editing issues

- Use `patch` rather than resending the description.
- Linear turns an issue ID (`RZG-N`, `RZE-N`) in a description into a link. A `patch` whose `old_string` contains an issue ID won't match. Anchor on text next to it instead.
- `replace_range`'s `to` is exclusive.

**Limits**
- The workspace is on Linear's free plan, which caps how many issues it can have. If creating an issue fails with "exceeded the free issue limit", tell the user. Run the archive sweep, then suggest archiving or deleting more. Don't work around it silently.

## The API key and views

- The key is in `~/.config/linear-api-key`. Read it only inside the command (`-H "Authorization: $(tr -d '[:space:]' < ~/.config/linear-api-key)"`, or the way `archive_closed.py` reads it). Never print it, log it or copy it anywhere.
- Endpoint: `https://api.linear.app/graphql`.
- **Show the user every write before you run it**, apart from the archive sweep (pre-approved above).
- **Views:**
  1. `customViewCreate` with `filterData`.
  2. Then `viewPreferencesCreate` (`viewType: customView`) twice: `type: organization` and `type: user`. The user's own preferences win.
  3. Set `showCompletedIssues: "all"` when the view shows closed issues.
  4. For collapsible parents, use `issueSubGrouping: "rootIssue"` with `issueNesting: "none"`.
  5. Check `viewPreferencesValues` afterwards.
- A view's link is `https://linear.app/rze/view/<slugId>`.
