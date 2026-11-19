# Verify: RZE

Read by the `verify` skill (`.claude/skills/verify/SKILL.md`), which holds the general rules and the order of the stages. Per repository, never synced. Building and driving the editor are covered by the `run-rze` skill (`.claude/skills/run-rze/SKILL.md`, its Linux section). CLAUDE.md's rules apply throughout (both platforms must build, no `git stash`, scoped restores, imgui.ini).

## 1. Build

The Linux Development build, warnings as errors:

```
make -C RZE -j"$(nproc)"
```

After restoring a file from a scratchpad copy, `touch` it before building, or the incremental build skips it.

## 2. Tests

```
./RZE/_build/development/UnitTests
```

Exit code 0 means all passed (`-tc="<pattern>"` runs matching test cases). Once for a full check, once for a small issue, and once more after rebasing, before landing. There's no counting runner yet, so a lane's tab shows the test-run sweep, not a count.

## 3. Compare with base

The editor check, off-screen so lanes don't fight over the desktop:

```
.claude/skills/run-rze/driver.py launch --harness --nested --scene <scene>
```

Take screenshots of the change (read each one), then close. The driver backs up and restores imgui.ini. No side-by-side with the base is required; take one only when the user asks or the change is a visual comparison.

Small issue: only when the change is visible in the editor.

## 4. Real launch

Covered by stage 3's launch. No timing here: A/B performance runs follow CLAUDE.md's Performance rules (on the desktop, never nested), and only when no other lane runs. If the change is perf-relevant, say so and offer the A/B for after the lanes are done.

## 5. Mutation checks

none required today (RZE's workflow doesn't ask for them). When you do one, restore from the scratchpad copy, `touch` it, and rebuild.

## 6. Other platforms

This machine builds Linux only. Name what Windows needs checked: DX11 paths, and MSVC-only rules in CLAUDE.md (for example no `[[maybe_unused]]` on a range-for variable). Windows checks are issues under the check parents (see `linear.md`).

## 7. Report

The Result note, before the Notes: what was done, changes from the Decisions, and the results, with numbers when relevant (build, UnitTests, screenshots read, what Windows needs checked). Then give the commit subject and description, and wait.
