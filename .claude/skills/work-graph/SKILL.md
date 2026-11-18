---
name: work-graph
description: Analyse the dependencies between this repository's Linear team's open issues (the team is in .claude/workflow/repo.md) and build a work graph of waves to feed into the lanes skill. Run only when the user explicitly asks for a work graph.
disable-model-invocation: true
---

# Work graph

Turn a team's open Linear issues into a **work graph**. The graph has the issues as nodes and "blocked by" as edges, and groups them into **waves** that can run side by side. The lanes skill then runs the waves.

**Only run this when the user explicitly asks** ("build a work graph", "organise the backlog into dependencies"). Never on your own initiative.

## 0. Which team

This skill is shared between repositories (kept in step with `sync-workflow`).

- The team is the one in `.claude/workflow/repo.md` (`<TEAM>`; `<main>` is the main checkout). Read the `linear` skill and `.claude/workflow/linear.md` first.
- If it's unclear, ask which team.

Run the archive sweep first, if `repo.md` turns it on (the command is there).

## 1. Gather

1. List every open issue of the team (backlog, todo, in progress, in review), page by page until `hasNextPage` is false.
2. For each issue, read it with `get_issue` (`includeRelations`), and read its comments with `list_comments`. Note:
   - its parent and children (an epic is done when its children are)
   - its existing blocked-by and blocks relations
   - whether it's a **placeholder** ("to be discussed", questions only): it starts with a discussion, not a build
   - the modules or areas it will touch (from its description, its spec sections and a quick look at the code)
   - anything the user said about order or holds ("do this later", "goes last")
3. Check what's already running: `git worktree list` (existing lanes), and issues In Progress or In Review.

## 2. Work out the edges

An issue is **blocked by** another when:
- it needs the other's code or design (a model, a widget, an API it builds on), or
- the user said it comes after it, or
- it's a sweeping change (a rename, a reformat, a package move) that has to go **last, alone**, so it's blocked by everything else in the graph.

Two issues that only **edit the same files** (the same panel, the theme, the main window, the layout model) aren't blocked by each other. They're **overlaps**: they can run side by side but must rebase carefully. Note them; don't make them edges unless the overlap is so large that running them together would mostly produce conflicts.

Keep edges minimal: if A ← B ← C, don't add A ← C.

**Classify** each node:
- **lane**: its own worktree and tab (lanes skill)
- **small**: a few files, no new module, no architecture change, about one short sitting. Done in the coordinator, or on the small-items lane, in order
- **discussion first**: a placeholder. Its lane starts with the review questions
- **epic**: not run itself. Close it when its children are done
- **held**: the user is doing it later. In the graph, but no lane until they say

## 3. Waves

- **Wave 1:** every node with no open blockers, except held ones. Keep it to about 3 to 5 lanes, plus the small items. Say which you'd defer if there are more.
- **Later waves:** each node starts as soon as everything blocking it has landed. Name the trigger ("after RZG-61 lands").
- **Final:** the last-alone nodes, when no other lane is running.

## 4. Show the user, then record

Show:
- an ASCII graph of the edges, with independent nodes and last-alone nodes listed apart
- a waves table: wave, lanes, small items, starts when
- notes: overlaps to rebase carefully, placeholders, likely epic splits, decisions only the user can make (a colour to pick, a hold)

Then **ask before writing anything to Linear**. Once the user agrees:
- add the blocked-by relations that aren't in Linear yet (`save_issue` with `blockedBy`)
- don't remove existing relations unless the user says so
- write the graph file (below). It replaces any earlier graph for the team; if one is still `active`, ask first.

## The graph file

`~/.claude/work-graphs/<TEAM>.md` (for example `RZG.md`). It outlives any one session, so a later `/lanes graph` can run it. **Linear holds the edges** (blocked-by relations), so the file never copies them: whoever runs the graph re-reads them from Linear. The file holds everything else:

```markdown
# Work graph: RZG

Status: active          <!-- active | done -->
Built: 2026-11-18

## Nodes

| Issue  | Title (short)          | Kind              | Notes                              |
|--------|------------------------|-------------------|------------------------------------|
| RZG-56 | Branch button          | lane              | overlaps RZG-62 (controls_panel)   |
| RZG-61 | Whole-file diff unified| small             |                                    |
| RZG-43 | Sorting in panels      | discussion first  |                                    |
| RZG-42 | Worktree window        | epic              | children: 47, 48, 49, 50           |
| RZG-51 | Window management      | held              | user: "done later"                 |

## Last alone

RZG-59: runs when every other node is done and no lane is running.

## Waves (as planned)

1. RZG-56, RZG-58, RZG-65; small: RZG-61 → RZG-55
2. RZG-62 (after 56), RZG-60 (after 61)

## Log

- 2026-11-18 built: 16 nodes
```

Kinds are `lane`, `small`, `discussion first`, `epic` and `held`. The waves are the plan as shown to the user; what actually runs next always comes from Linear's relations.

## 5. Hand off to lanes

- The lanes skill (`<main>/.claude/skills/lanes/SKILL.md`) runs the graph with `/lanes -N graph`. Tell the user that command; start wave 1 only on their go.
- If the repository has no lane prompt yet (`.claude/workflow/lane-prompt.md` is missing), stop after step 4 and say so.

## Rules while the graph runs

These are the user's standing rules; they apply to whoever coordinates the lanes.
- **Only graph nodes get lanes.** A task created along the way, by the user or as a lane's follow-up, joins the graph only if it **directly relates** to graph work: a split of a graph issue into an epic's children, or an issue that blocks a graph issue. Everything else goes to the backlog, with no lane and no link to the graph's last-alone node, until the user asks for it.
- When a landing unblocks a graph node of the wave already running, start it, and say so in one line. Stay silent when nothing was unblocked.
- **Waves are gated:** before the first node of a new wave starts, tell the user which wave and which nodes, and wait for their go.
- When an epic's children are all done, ask the user before closing it.
- The user can change the graph at any time: hold, reorder, add or drop nodes. Update the graph file and the Linear relations to match, and add a line to its Log.
