# Workflow settings: RZE

The shared workflow skills (`lanes`, `work-graph`, `linear`, `sync-workflow`) read this file for everything that
differs between repositories. It's per repository and never synced.

| Setting            | Value                                                        |
|--------------------|--------------------------------------------------------------|
| Repository         | RZE (C++17 engine and the RZEStudio editor)                  |
| Main checkout      | `/home/JPOD/dev/RZE`                                         |
| Linear team        | `RZE` (workspace `rze`)                                      |
| Archive sweep      | on: `python3 <main>/.claude/skills/linear/archive_closed.py --team RZE` |
| Lane name          | `RZE-N` (the issue ID)                                       |
| Lane worktree      | `/home/JPOD/dev/RZE-RZE-N`, branch `RZE-N`                   |
| Small-items lane   | worktree `/home/JPOD/dev/RZE-small`, tab `RZE-small`, hook name `small` |
| Test runs          | `/UnitTests(\.exe)?\b` (a regular expression over a lane's process command lines; see the lanes skill's "Test runs") |
| Lanes by default   | yes: lanes is the default way to work an RZE issue          |

`<main>` in the shared skills means the main checkout above.

## Landing

A lane lands on the branch the main checkout is on when the lane starts (`{{BASE}}` in the lane prompt): the current
effort branch (for example `look-what-claude-can-do-batch-refactor`), not necessarily `master`.

The main checkout usually holds files the user keeps uncommitted (`RZE/Config/imgui.ini`, `Editor.args.json`, scene
edits such as `Sponza.scene`, `.vscode/`). They don't block a fast-forward unless a lane's commit touches the same
file; never stage, stash or restore them.

## Small issues

Small means a few files, no new module, no change to the architecture, and about one short sitting. Its verification
is the Linux Development build and `UnitTests`; the editor check only when the change is visible in the editor.

## Overlap hints for planning

Issues that edit these overlap and must rebase carefully:
- the Rendering module and its stages, `GraphicsLayouts.h`, the shaders of one stage
- `EditorStyle` and the editor framework (`UI/Framework`), the same editor panel
- Sharpmake files (`*.sharpmake.cs`): any lane that adds or removes a source file reruns `GenerateSolutions.sh`
- `Docs/Generated/Engine` pages

Builds are heavy: each lane's worktree has its own `_build`, and parallel builds share the CPU. Keep N low (2–3)
unless the user says otherwise.

## Rules every lane carries (they're in `lane-prompt.md`; keep them in sync)

- Set up the worktree before building: `GenerateSolutions.sh`, the Development build, `SourceAssetBurner` (generated
  projects and `ProjectData` are gitignored).
- Specs in `Docs/Plans/` are gitignored: read them from the main checkout, `/home/JPOD/dev/RZE/Docs/Plans/`.
- Both platforms must build. A lane builds Linux only and names what Windows needs checked.
- No A/B performance numbers while other lanes run; offer the A/B for after the lanes are done.
- RZE's git rules: never `git stash`; scope checkouts and restores to exact files; never stage `imgui.ini`,
  `Editor.args.json` or `Docs/Plans/`.
- The user's standing rules: short replies; no `rm -rf` on variable paths; the Linear section is "Notes"; a parent
  goes In Progress with its child; never print the Linear API key; RZE's Linear conventions (`linear.md`).
