---
name: lanes
description: Run several Linear issues in parallel as "lanes" - one git worktree, one Konsole tab and one Claude session per issue, each named after its issue ID (e.g. RZG-N). Where .claude/workflow/repo.md says "Lanes by default: yes", this is the default way to work an issue: use it whenever the user asks to work on, start or do one, except a small issue (done in the coordinator session, or on the small-items lane) or when they ask to do it in the current session. Also for planning which issues can run side by side.
argument-hint: "[-N] [graph | <TEAM>-n ...]"
---

# Lanes: parallel issues in Konsole tabs

This skill is shared between repositories (kept in step with `sync-workflow`). **Read `.claude/workflow/repo.md` first.** It gives:

- `<main>`: the main checkout; `<TEAM>`: the Linear team key
- the lane name, worktree path and branch (below: `<TEAM>-N` and `<wt>`)
- the small-items lane's worktree and tab title
- what counts as a small issue, overlap hints for planning, and the rules every lane carries

The lane prompt is `.claude/workflow/lane-prompt.md`, also per repository.

One lane works on one Linear issue. A lane is named after its issue's ID, **`<TEAM>-N`**. That name is used for:

- its git worktree `<wt>` (the path in `repo.md`)
- its branch: `<TEAM>-N`
- its Konsole tab title: `<TEAM>-N`

Each lane is a Claude session in its own tab. It runs the issue's normal per-issue workflow with the user (review, plan, build, verify, report, commit) and lands its commit by fast-forward on **its base**: the branch the main checkout was on when the lane started.

This session is the **coordinator**. It plans the lanes, starts them, starts lanes that were waiting, and cleans up. It doesn't do lane work itself, apart from small issues (step 0). While lanes run, it never commits in the main checkout, edits it, or switches its branch: lanes fast-forward their base there.

## Arguments

`/lanes -N …` sets **N, the most lanes running at once** for this invocation; for example `/lanes -4` allows four. It counts every lane tab this coordinator has open, including the small-items lane, but not lanes from other sessions. Without `-N`, the limit is 3. The rest of the arguments name the issues to run (`/lanes -2 RZG-12 RZG-14`); with none, use the issues the user names in the request.

While lanes run, the limit holds: when a lane lands and frees a slot, start the next waiting issue; when every slot is full, an unblocked issue waits for one. Tell the user in one line when an unblocked issue is waiting for a slot. The user can change N at any time ("allow 5 lanes").

## Running a work graph (`/lanes -N graph`)

`graph` runs the team's work graph, built with the work-graph skill: `~/.claude/work-graphs/<TEAM>.md`. If the file is missing, or its Status isn't `active`, say so and stop.

- **Linear holds the edges.** Never trust a copy: each time you decide what to start, read each open node's blocked-by relations from Linear. A blocker counts as cleared when it's Done or Canceled, or archived. Pick up any change the user or a lane made there.
- **What can start:** a node whose blockers are all cleared, that isn't `held` or an `epic`, and that has no lane yet. Start them in wave order, up to N lanes (Arguments). **Waves are gated:** before starting the first node of a new wave, tell the user which wave and which nodes, and wait for their go; within a wave they've started, start nodes as their blockers clear. Follow any rules the graph file adds (for example a node that runs first, alone). A `small` node goes to the small-items lane, in order, or to the coordinator if the user prefers. A `discussion first` node's prompt says its lane starts with the review. The last-alone node starts only when every other node is done and no lane is running.
- **Catching landings:** run a background Bash that exits when the base branch moves (`X=$(git -C <main> rev-parse <base>); until [ "$(git -C <main> rev-parse <base>)" != "$X" ]; do sleep 5; done; git -C <main> log --format=%s "$X"..<base>`). Each time it exits, work out what to start, start it, and re-arm the watch. Say so in one line when you start something; stay silent when nothing was unblocked.
- **Only graph nodes get lanes.** A new task joins the graph only if it directly relates to graph work: a split of a node into an epic's children, or an issue that blocks a node. Add it to the file's Nodes with a Log line. Everything else goes to the backlog, with no lane.
- **Epics:** when all of an epic's children are done, ask the user before closing it.
- **Keep the file current:** log each landing, each lane started, and each change the user makes (a hold, a reorder, a dropped node). When every node is done, set Status to `done` and tell the user.

## 0. Lane or coordinator?

**Small issues are done in the coordinator session, not as a lane.** `repo.md` says what small means. Everything else is a lane. When unsure, tell the user which you'd pick and why.

A small issue follows the same per-issue workflow, with the lighter verification `repo.md` gives. While lanes are running, the coordinator does it in its own worktree (`<wt>`, branch `<TEAM>-N`, with no tab), and lands it with `merge --ff-only` like a lane. Its changes must never sit in the main checkout, because a dirty main checkout makes the lanes' fast-forward landing fail.

**The small-items lane.** The user may instead put the small issues on one lane of their own, worked strictly in the order given:
- its worktree (from `repo.md`), created with `git -C <main> worktree add -q --detach <small wt> <base>`, then `git -C <small wt> switch -q -c <TEAM>-<first>`
- its tab title is in `repo.md`; the lane's name in the hooks (step 2) is `small`
- one branch `<TEAM>-N` per issue, each started from the current base; each issue lands, and the user agrees to move on, before the next one starts
- its prompt is `lane-prompt.md` with `{{SMALL_LANE}}` filled in (step 2)

## 1. Plan (wait for the user)

1. Gather the issues the user named (or the team's open issues, if they asked for "the remaining ones"). Read each one with `get_issue` and its comments, including parents and children.
2. Check what is already running:
   - `git worktree list`: existing lanes, and other sessions' worktrees (for example `.claude/worktrees/*`). Never touch those.
   - `git status`: the main checkout must be clean, apart from untracked or already-modified files the user keeps there. Name any you find; lanes never stage them.
   - `git branch --show-current`: the **base** the lanes will land on. Say it in the plan.
   - `git fetch -q && git status -sb`: the base is in sync with its upstream, or note that it is ahead.
3. Work out dependencies and overlap:
   - which issue needs another's code
   - which issues edit the same modules (the overlap hints in `repo.md`)
4. Propose the lanes in a short table: lane (`<TEAM>-N`), issue title, what it may touch, and what it waits for. A lane that depends on another either:
   - **starts later**: the coordinator starts it once the other has landed; or
   - **starts now, lands after**: it builds the parts that don't depend on the other, and lands only after the other's commit is on the base. Its prompt says so.

   Keep it to N lanes at a time (see Arguments; 3 if not given).
5. **Wait for the user's go.** Don't create worktrees or tabs before it.

## 2. Start the lanes

For each lane `<TEAM>-N` that starts now, with `<base>` the main checkout's current branch:

```bash
cd <main>
BASE=$(git branch --show-current)
git worktree add -q -b <TEAM>-N <wt> "$BASE"
```

Write its prompt from `<main>/.claude/workflow/lane-prompt.md` into the session's scratchpad as `<TEAM>-N.md`, without its first-line comment. Fill in:

- `{{LANE}}`: `<TEAM>-N`
- `{{ISSUE}}`: the issue's ID and title
- `{{BASE}}`: the base branch
- `{{OTHER_LANES}}`: one line per other lane: ID, title, and the areas it touches
- `{{DEPENDS}}`: for "starts now, lands after", say which issue must be on the base first, and which parts may be built before it lands; otherwise "None."
- `{{SMALL_LANE}}`: empty for a normal lane. For the small-items lane, use `{{LANE}}` = `small`, `{{ISSUE}}` = the issues in order, and this text, with any per-issue notes (a colour to pick, overlap with another lane):

  ```
  ## Small-items lane
  This lane runs several small issues one after another, in this order: <TEAM>-a, <TEAM>-b, …
  - Your worktree is <small wt>. Wherever this prompt says branch `small`, or a commit title or merge of `small`, use the current issue's own `<TEAM>-N` instead.
  - Small issues: the same workflow, with the lighter verification for small issues.
  - For each issue: `git switch -c <TEAM>-N {{BASE}}`, do the whole workflow, land it, then `git switch --detach {{BASE}}` and `git branch -d <TEAM>-N`. Start the next issue only after the previous one has landed and the user agrees to move on.
  ```

**The attention hooks.** Each lane's Claude session runs with a settings file that tells the coordinator when the lane needs the user. `<dir>` is `<scratchpad>/lanes`, shared by all of this coordinator's lanes. Write `<dir>/settings-<lane>.json` (`<lane>` is `<TEAM>-N`, or `small`):

```json
{"env": {"LANE_DIR": "<dir>", "LANE_NAME": "<lane>"},
 "hooks": {
  "Stop": [{"hooks": [{"type": "command", "command": "python3 <main>/.claude/skills/lanes/lane_hook.py <dir> <lane> stop"}]}],
  "Notification": [{"hooks": [{"type": "command", "command": "python3 <main>/.claude/skills/lanes/lane_hook.py <dir> <lane> notify"}]}]
}}
```

The `env` lets the lane's own done command (Attention highlights) find its state file.

Then open the tab and start Claude in it. This is Konsole's D-Bus API on this window (see the memory note "Konsole tabs via D-Bus"). `runCommand` needs Konsole's "Enable the security sensitive parts of the DBus API" setting, which the user has turned on.

```bash
P=<scratchpad>/<TEAM>-N.md
SET=<scratchpad>/lanes/settings-<TEAM>-N.json
HERE=$(qdbus-qt6 "$KONSOLE_DBUS_SERVICE" "$KONSOLE_DBUS_WINDOW" currentSession)
ID=$(qdbus-qt6 "$KONSOLE_DBUS_SERVICE" "$KONSOLE_DBUS_WINDOW" newSession "" <wt>)
qdbus-qt6 "$KONSOLE_DBUS_SERVICE" "$KONSOLE_DBUS_WINDOW" setCurrentSession "$HERE"   # the new tab doesn't take focus
qdbus-qt6 "$KONSOLE_DBUS_SERVICE" /Sessions/$ID setTabTitleFormat 0 "<TEAM>-N"
qdbus-qt6 "$KONSOLE_DBUS_SERVICE" /Sessions/$ID setTabTitleFormat 1 "<TEAM>-N"
qdbus-qt6 "$KONSOLE_DBUS_SERVICE" /Sessions/$ID setTitle 1 "<TEAM>-N"
qdbus-qt6 "$KONSOLE_DBUS_SERVICE" /Sessions/$ID runCommand "claude --settings $SET \"\$(cat $P)\""
```

The tab title format is set to the literal lane name, so Claude's own terminal title doesn't replace it.

**Check that Claude started.** `runCommand` sent before the new tab's shell is ready is lost. This happened to one of two tabs. Check for a `claude` child of the tab's shell, and send the command again if there is none:

```bash
pgrep -a -P "$(qdbus-qt6 "$KONSOLE_DBUS_SERVICE" /Sessions/$ID processId)" | grep -q claude \
  || qdbus-qt6 "$KONSOLE_DBUS_SERVICE" /Sessions/$ID runCommand "claude --settings $SET \"\$(cat $P)\""
```

- If `KONSOLE_DBUS_SERVICE` is unset (not running in Konsole), or `runCommand` is refused, give the user the exact command to run in a new tab: `cd <wt> && claude --settings <SET> "$(cat <P>)"`.
- Don't retry temporary Konsole profiles: a running Konsole ignores them.

Tell the user which lanes started (the tab, the session ID and the base). They review and approve each lane in its own tab.

**Start the watch**, if it isn't running yet: a Monitor running `python3 -u <main>/.claude/skills/lanes/watch.py <scratchpad>/lanes 2>&1`, with the maximum timeout (30 minutes). Re-arm it each time it expires, for as long as lanes run.

## 3. While lanes run

- **When a lane needs the user, tell them.** Each watch event is a lane that finished its turn (questions, a plan waiting for "start", a report), is stuck on a permission prompt, or is free. Reply with just one line naming the lane and what it needs ("RZG-58 needs you: six review questions"), or that it's free ("RZG-58 landed and is free"). Don't mention the monitor or the event.
- Tab colours and events follow the attention highlight rules below.
- When the user says a lane has landed, check `git -C <main> log --oneline <base>`. Then start any lane that was waiting for it (step 2).
- If the user switches the main checkout to another branch while lanes run, lanes started on the old base stop before landing and ask (their prompt says so). New lanes take the new branch as their base.
- If the user asks about a lane, read its issue in Linear (state, Result note) and `git -C <worktree> log`/`status`. Don't guess.
- Don't push unless the user asks. When they do, push the base from the main checkout (`git push origin <base>`), after checking it is a fast-forward of its upstream.

## Attention highlights

How a lane tells the coordinator it needs the user, and how its tab shows that. `lane_hook.py` and `watch.py` implement these rules; change the scripts and this section together.

**What counts as needing the user** (`lane_hook.py`, run by the lane's own hooks):
- **Stop**: the lane finished its turn. The event reads `waiting for you: <the end of its last message>` (up to 200 characters).
- **Notification**: only a **permission prompt** counts (`needs permission: <message>`). Idle reminders are ignored, since they repeat the Stop event.
- **Done**: the lane runs `python3 <main>/.claude/skills/lanes/lane_hook.py "$LANE_DIR" "$LANE_NAME" done` as its last landing step, once its commit is on the base and its issue is Done and archived (the lane prompt says so). It records the state `done` silently; the Stop that ends that turn keeps it `done` and reports `free: <the end of its last message>`. A Stop counts as free only while the user hasn't answered since the done command.
- Each event appends one line to `<dir>/attention.log` (`HH:MM <lane> <what>`) and records `<dir>/state/<lane>.json`: the lane's Konsole session (`KONSOLE_DBUS_SESSION`), its transcript, the transcript's size at that moment, the state (`attention` or `done`), and the event.
- **An answer** is a `user` record in the transcript that carries the user's words. Tool results are stored as `user` records too and don't count (`lane_state.py`).

**Tab colours** (`watch.py`, run by the coordinator as a Monitor):

| Colour | Hex       | When |
|--------|-----------|------|
| red    | `#c0392b` | the lane needs the user: a Stop or a permission prompt they haven't answered |
| orange | `#d68910` | the lane is working: the user has answered (a new answer in its transcript after the recorded size) |
| blue   | `#2f6fd0` | the lane stopped, but one of its own shell commands is still running (a background task), so it's waiting on that, not the user |
| green  | `#3b8f5a` | the lane has landed its work and is free for more (the done command). The small-items lane: this issue landed, ready for the next |

- An answer turns red or green into orange: a lane the user writes to again is working, and its next Stop is red as usual.

- Blue applies only to a Stop. A permission prompt stays red even while a shell command runs. The check is a `shell-snapshots` child of the lane's `claude` process under the tab's shell.
- A tab is recoloured only when its colour changes. A lane with no Konsole session in its state file gets events but no colour.
- The watch needs `KONSOLE_DBUS_SERVICE`; outside Konsole it can't run. Say so, and read `attention.log` yourself instead.

**The coordinator's side:**
- Each line the watch prints is one event: reply in one line, as in step 3.
- The watch starts reading `attention.log` from its current end. When you re-arm it after it expired, read the log lines written since the last event you saw, so nothing is missed.
- A lane started without the hooks (no `--settings` file) gets neither colours nor events: say so.
- Removing a lane's state file (step 4) stops the watch colouring its tab. The tab keeps its last colour.

## 4. Clean up (when the user says the lanes are done)

For each finished lane:

```bash
cd <main>
git worktree remove <wt>     # literal path, never a variable or glob
git branch -d <TEAM>-N       # -d: refuses if not merged; then ask
```

- If `remove` refuses (uncommitted changes), show the user what is left. Don't force it.
- Remove the lane's `<scratchpad>/lanes/state/<lane>.json`, so the watch stops colouring its tab. It is a literal path, so a plain `rm` is fine.
- The small-items lane's worktree has no branch of its own: remove it once its last issue has landed, and delete each `<TEAM>-N` branch as it lands.
- Close the lane's tab only if the user asks, and only once its Claude session has exited: `qdbus-qt6 "$KONSOLE_DBUS_SERVICE" /Sessions/<ID> runCommand exit`. A Konsole session has no `close` method over D-Bus.
- Never `rm -rf` a worktree, or any path built from a variable or a glob.

## Rules for every lane (they're in `lane-prompt.md`; keep them in sync)

- Lanes follow the `linear` skill and `.claude/workflow/linear.md`. The prompt points at the main checkout's copies by absolute path, so a lane always reads the current ones.
- One issue per lane (the small-items lane: its issues in order, one at a time), and the team's per-issue workflow (`linear.md`): review, Decisions, plan, wait for "start", build, verify, report, commit only on the user's yes.
- Landing: check the main checkout is still on the base (stop and ask if not), `git rebase <base>`, the verification once more, one commit in the repository's style with the Co-Authored-By line, then `git -C <main> merge --ff-only <TEAM>-N`. Never force anything, never merge non-fast-forward, never commit in the main checkout. After the issue is Done and archived, the done command (Attention highlights) marks the lane free.
- The repository's own lane rules are listed in `repo.md`.
