# Linear conventions: RZE (team RZE)

Per repository, never synced. The shared rules (archiving, the API key, views, editing quirks) are in
`.claude/skills/linear/SKILL.md`. Linear is the source of truth for RZE tasks.

**Workspace and teams**
- The workspace is `rze` (urlKey `rze`). Team **RZE**, key `RZE`. Issue identifiers `RZE-<n>` are assigned by Linear.
- **RZG** is RZGit's team. Don't work its issues from here.
- Projects: Rendering, RZEStudio, Asset Pipeline, GameObject infrastructure, Cleanup, MeshEditor, RZE Hub, Linux,
  Windows, Engine Core, Build, Test.
- **If an item's project or shape is unclear, ask before creating it**, not after.

## Writing an issue

- **Check for a duplicate first** (`list_issues` with a `query`), across all states.
- **Title:** `[<Project name>] <Title>`: the project's exact name in brackets, sub-issues too. **Never put `RZE-<n>` in
  a title**: Linear already shows the identifier, and it doubles the generated branch name.
- **Project choice:** anything only one OS has goes in **Linux** or **Windows**, whatever its subject ("platform
  wins"). Engine-wide systems → Engine Core; build and toolchain → Build; tests, harness, drivers, the run-rze skill →
  Test.
- **Labels:** `Claude` plus one type label (`Bug`, `Feature`, `Improvement`, `Investigation`). Never a
  "deferred"-style label or wording. New items start in Backlog unless they're next.
- **Big work** is a parent issue with sub-issues (Linear has no epics). A sub-issue may sit in another project than its
  parent.
- **Body**, for people to read: short sentences, the same word for the same thing. Detail for agents (plans, file
  lists, logs) goes last in a collapsible section named **Notes**: `+++ Notes` … `+++`. Never "Agent notes".
- **Images** the user shares are never attached to issues. Record what you derive from them as text.

## Links

- **Every mention of an issue is a link**: `[RZE-<n> <title or short name>](<https://linear.app/rze/issue/RZE-<n>>)`.
  Done and Canceled issues included: search all issues, not just open ones, before writing a mention as plain text.
- **Link text carries the ID.** A list of steps (a parent's work order, "do these first") is one linked issue per line,
  the link text being that issue's exact title: no checkboxes, no bullets. A step inside a sentence links its short
  name (`[RZE-230 S0b](…)`).
- Only what has no issue at all stays plain text.
- When creating a set of issues, create them first, then patch in the links once every identifier exists. When an issue
  joins a parent's order, add its line to the parent's list too.
- Linear stores links as `[text](<url>)` with `[` escaped in the text: match that form when patching. Inside `~~…~~`
  it breaks code spans: write struck text without backticks.

## Priorities and contention

- **A prioritize pass whenever you create issues, parents or sub-issues.** Set a priority on each and re-check its
  neighbours:
  - Urgent: the work in flight on the current branch
  - High: its next steps, bugs that lose data or break input, Windows checks of DX11 render paths
  - Medium: planned features and fixes with no hard order
  - Low: speculative, gated or cleanup
  - A parent takes its highest child's priority. Done and Canceled issues keep what they had.
- **Contention:** where issues contend for a slot (same tier, same files, or an order that depends on a number not yet
  measured), comment on each of them linking the others, one titled link per line, with a "Default until resolved"
  line. The user resolves them when they come up.
- **When a priority or order is decided:** set the priority, wrap every line of the contention comment in `~~…~~` (edit
  it, don't delete it), and reply in its thread with "**Resolved (date): …**": what was decided, by whom (a quote), the
  numbers behind it, and the new order.
- **Then follow the references:** every other issue whose contention comment names the decided one. Strike only what the
  decision settled, keep what's still open, and reply "**Partly resolved (date): …**" with what was decided, what is
  still open, and a new default if the old one was struck. Leave a comment that stays true untouched.

## Working an issue

The rules in CLAUDE.md ("Working together") apply; in Linear terms:

1. **Review:** findings and a plan first. Go through open questions with the user, each with a recommendation, and
   push back where there's a decision-relevant case. Record the answers under `**Decisions** (date)` in the issue.
2. **Plan:** in implementation order. Wait for "start".
3. **Build:** set the issue **In Progress**, and its parent too.
4. **Verify:** the Linux Development build (warnings are errors), `UnitTests`, and the editor check. Name what Windows
   needs checked. Numbers when relevant.
5. **Report:** set it **In Review**. Add `**Result (date, uncommitted):**` before the Notes: what was done, changes from
   the Decisions, and the results. Give the commit subject and description, and wait.
6. **Commit** only when told ("commit"):
   - RZE's subject style, `<Area>: <subject>` (e.g. `Mesh pass performance S2: batches sorted by state`), no issue ID
     in the subject; the description ends with a link to the issue and the Co-Authored-By line
   - check `git log -1`, set the issue **Done**, and replace "uncommitted" with the hash
   - then **archive** (the linear skill)
   - ask whether to update `Docs/Generated/Engine`, naming the pages likely affected
   - push only when the user asks

**Windows checks** are issues under the check parents (RZE-225..229); `.claude/WindowsChecks.md` only holds the run
procedure.

**Parents:** when all of a parent's sub-issues are done, ask the user before closing it. Closing it archives it with
all its children.
