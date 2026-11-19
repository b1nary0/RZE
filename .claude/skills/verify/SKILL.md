---
name: verify
description: How to verify a change before reporting it - the general rules and the fixed order of stages (build, tests, compare with base, real launch, mutation checks, other platforms, report), filled in by this repository's .claude/workflow/verify.md. Use at the Verify step of the per-issue workflow (lanes, the coordinator's small issues, any session), and whenever the user asks to verify or check a change.
---

# Verify a change

This skill is shared between repositories (kept in step with `sync-workflow`). It holds what's true everywhere: the rules, and the stages in their order. **What each stage runs is per repository**, in `<main>/.claude/workflow/verify.md` (`<main>` is the main checkout, from `.claude/workflow/repo.md`). Read that file each time you verify, from the main checkout, so you always follow the current one.

## The stages

Go through them in this order. `verify.md` has one section per stage, with these exact headings. For each stage, do what the section says. A section that says `none` is skipped: say so in the report, with the reason it gives.

1. **Build**: compile the change, with warnings as errors.
2. **Tests**: the suite, run the number of times the section gives for this case: a full check, a small issue, or the rerun before landing.
3. **Compare with base**: what is compared against the branch you'll land on (the base), and how.
4. **Real launch**: start the program for real, the way a user would, and time it where the section says.
5. **Mutation checks**: break each key piece of the change and confirm a test fails, then restore it.
6. **Other platforms**: what this machine can't check, and what to name for someone who can.
7. **Report**: what goes in the issue's Result note.

A small issue (as `repo.md` defines it) takes the lighter version where a section gives one.

## Rules for every stage

- **The base:** compare against a temporary worktree of the base, made in a fresh `mktemp -d` folder (`git worktree add --detach <that folder>/head <base>`), and remove it afterwards with `git worktree remove <literal path>`.
- **Fresh folders:** settings, scratch clones and outputs go in fresh `mktemp -d` folders under your session's scratchpad. Never clear an old folder, and never `rm -rf` a path built from a variable or a glob.
- **Never write to a real repository** you're testing against: anything that writes uses a scratch clone (`git clone --no-hardlinks <repo> <fresh folder>/…`). A local value such as the test repository comes from the repository's local settings when `verify.md` names one; if it isn't set, stop and ask the user, never guess.
- **Restore** temporary code (mutation checks, debugging) from a scratchpad copy of the file, never with `git checkout` or `git stash`.
- **Tests through the runner:** when `verify.md` names a test runner, every test run goes through it, at every stage and while building too, targeted runs included (name the tests), so the lane's tab shows the count (the lanes skill's "Test runs"). Never call the test framework directly.
- **Timings and performance:** other lanes may be running builds, launches and suites at the same time. Say timings are noisy, and leave A/B performance comparisons until no other lane runs.
- **Numbers:** report results as numbers where they exist: tests run and passed, runs, timings, pixels that differ.
- **A stage that fails** stops the report: fix the change, or tell the user what failed and why, before going on.

## Writing a repository's verify.md

`<main>/.claude/workflow/verify.md`, per repository, never synced. Start with a line saying it's read by the `verify` skill, then the seven sections, in order, with these headings:

```markdown
## 1. Build
## 2. Tests
## 3. Compare with base
## 4. Real launch
## 5. Mutation checks
## 6. Other platforms
## 7. Report
```

Each section gives exact commands (absolute paths into the main checkout where it matters), the small-issue version if there is one, or `none` and why. Keep the rules above out of it: write only what's specific to the repository.
