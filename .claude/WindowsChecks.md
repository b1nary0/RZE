# Windows checks

Checks that only a Windows machine can do, for changes made and tested on Linux. **The checks themselves are Linear
issues** in the RZE team's **Windows** project, titled `[Windows] Check <ID>: ...`. Each is a sub-issue of one of these
parents (the same groups this file used to hold):

- **RZE-225** Checks, harness: changes made on Linux (the `driver.ps1 -Harness` checks)
- **RZE-226** Checks, hands-on session 1: Visual Studio and the debugger (person)
- **RZE-227** Checks, hands-on session 2: the Game (person)
- **RZE-228** Checks, hands-on session 3: the Editor with real input (person)
- **RZE-229** Checks, hands-on session 4: CubeMap on DX11 (person)

This file keeps how to run them and the steps every run starts with. **Linux Claude maintains both**: it adds a check
issue for every change it can't verify on Windows, and closes it once it passes. **Windows Claude runs them** when the
user asks ("follow `.claude/WindowsChecks.md`"). Windows Claude reads and updates the issues through the Linear MCP
server (`linear`, registered at user scope with `claude mcp add`); if its tools aren't available, stop and ask the user
to set it up rather than working from an old copy of the checks.

## How to run this file (Windows Claude)

1. **If `git ls-files RZE/Config/imgui.ini` prints the file, copy `RZE\Config\imgui.ini` to `%TEMP%` first**, and
   put it back after the pull: the pull that untracks it deletes it (or stops, if it has local changes). Then
   `git pull`, then note the commit you're testing (`git log -1 --oneline`). If a check compares against the old build
   (e.g. a burn's hashes), do its "before" part now, with the binaries already in `_build`, before building.
2. Do **Every run** below, then the open check issues: the sub-issues of RZE-225, then the hands-on sessions
   (RZE-226 to RZE-229), each in identifier order. Use the `run-rze` skill (`driver.ps1`, `-Harness` where a check
   doesn't need the real mouse) for anything in the Editor. `driver.ps1` backs up `RZE/Config/imgui.ini` (local and
   untracked) at `launch` and puts it back after the session; if you start the Editor any other way, back it up first
   and restore it at the end (CLAUDE.md). Group checks that use the same scene and configuration into one session.
3. Record each result as a **comment on its issue**: the date, the commit, `pass` / `fail` / `skipped`, and what you
   saw. For a failure, give the exact error, the screenshot's path, or the log lines. Record **Every run**'s results
   (A1, A2, A3, Smoke) in one comment on RZE-225. Report failures; the user decides who fixes them.
4. Leave the issues open, and their state alone: Linux Claude reads the comments and closes the checks that passed,
   or turns a failure into a work item. If a check's steps no longer match the code, say so in its comment. Don't
   commit unless the user says so (CLAUDE.md).

Hands-on sessions (person) need someone at the machine (judging a picture, a GUI tool, a physical key, a debugger): do
what you can, then list the rest for the user.

Things to know on this machine (2026-11-10):
- The GPU is an **NVIDIA GeForce RTX 4070 Ti** (on 2026-11-08 it was a VMware VM with `VMware SVGA 3D`). Memory
  figures from before 2026-11-10 came from the VM, whose driver kept textures in system memory, so they don't compare
  with this machine's: measure a "before" here (e.g. a temporary revert) instead.
- The Editor's harness client area is 2560x1392. Measure coordinates from a `shot`.
- `driver.ps1` has no `move` command (`driver.py` has one). To hover the scene view without clicking, use
  `scroll -X <x> -Y <y> -Notches 0`. The Editor only takes keys (W/E/R, Esc) while the pointer is over the scene view.

## Every run

- **A1. Generate and build.** `GenerateSolutions.bat`, then build Debug, Development and Retail (`/W3 /WX`). Expect no
  errors or warnings.
- **A2. Unit tests.** `UnitTests.exe` in `RZE\_build\debug`, `development` and `retail`: exit code 0. Afterwards
  `RZE\_build\debug\ProjectData` and `RZE\_build\development\ProjectData` don't exist. Expected now: 493 test cases
  in Debug and Development, 485 in Retail. Linux runs 9 more, in `Utils\Linux` and `Rendering\OpenGL`, which Windows
  doesn't build. The counts in the check issues are Linux's from when each was written; this one is current.
- **A3. Burn.** `RZE\_build\debug\SourceAssetBurner.exe`: exit code 0, no `[Error]` line.
- **Smoke.** `driver.ps1 launch -Harness -Config debug -Scene Sponza.scene -SettleSeconds 20 -AppArgs
  '-gfxliveobjects'`, `shot`, `close`: the scene renders, `closed code=0`, the log has no `[Error]` line and ends
  "encountered 0 live resources". Shaders compile at start-up, so this is where an fxc failure shows. **A8.4:** the log
  also says "No D3D11 objects outlived shutdown besides the device."

## Earlier results

Results from before the checks moved to Linear (2026-11-14). New results go in comments on the check issues.

Results already read by Linux Claude are removed from this table; they're in this file's git history (the last full
table is at b17556af).

All 2026-11-08 rows are at eaf66bf4, those marked "+ fix" with the D10 fix (b17556af) applied. The 2026-11-10 rows
are at a979e2f1, on the RTX 4070 Ti (see "Things to know").

| ID | Date | Commit | Result | Notes |
|---|---|---|---|---|
| A1 | 2026-11-10 | a979e2f1 | pass | `GenerateSolutions.bat` (only Sharpmake's regex-separator warnings), then Debug / Development / Retail: 0 warnings, 0 errors. |
| A2 | 2026-11-10 | a979e2f1 | pass | Debug and Development 459 / 459 cases (22065 assertions), Retail 458 / 458 (22061); exit code 0 each. No `ProjectData` left in `_build\debug` or `_build\development`. |
| A3 | 2026-11-10 | a979e2f1 | pass | Debug burner: exit code 0, 15 sources imported, no `[Error]`. |
| Smoke | 2026-11-10 | a979e2f1 | pass | Debug Sponza, harness, 20 s: renders textured and shadowed, `closed code=0`, no `[Error]`, "encountered 0 live resources". |
| D12 | 2026-11-10 | a979e2f1 | pass | Debug (validation on, info queue live: no "isn't available" warning), settled 20 s / 8 s: **0** `D3D11 (352)` lines, and no `D3D11 (` line at all, on RenderPerfTest, RenderTest, DrawLineTest and Sponza. Sponza textured, floor tiles and arch stone repeating across surfaces. Every run `closed code=0`, 0 live. |
| D13 | 2026-11-10 | a979e2f1 | pass | D7's VM figures don't compare (other GPU), so "before" was measured here with `MEM_ARENA_SIZE` temporarily back at 2048 (reverted, rebuilt). Sponza settled 20 s, before → after: **Debug** private 5556 → 1586 MB (−3970), working set 4190 → 229 MB (−3961); **Development** private 5559 → 1579 MB (−3980), working set 98 → 101 MB. Expected −3968 MB (2 × 1984). Status bar `/ 64.00 MB`; peak **12090 KB** on Sponza (Linux's exactly), **2303 KB** on RenderPerfTest (Linux 3006; 947 KB last frame). No `arena was full` / `exceeds the arena` in any log; all `closed code=0`, 0 live. |
| F2 | 2026-11-10 | a979e2f1 | measured | Development `/Ob1` (as committed) vs `/Ob2` (`ConfigureRelease` temporarily `AnySuitable`, regenerated, `/t:Rebuild`; reverted and rebuilt). `Editor.exe` 2953728 → 3076096 bytes. Editor on monitor 1, `RenderPerfTest.scene`, 20 s warm-up, 30 s capture, alternated 3 runs a side, `imgui.ini` restored before each; about 13100 frames a run. **Update ms**, Ob1 → Ob2: mean 1.055 → 0.956 (**−9.4 %**), median 1.027 → 0.962 (−6.4 %), p95 1.446 → 1.347 (−6.9 %), p99 1.594 → 1.485 (−6.8 %); run means Ob1 1.040 / 1.084 / 1.040, Ob2 0.947 / 0.956 / 0.963, so every Ob2 run beat every Ob1 run. **Frame ms**: mean 2.290 → 2.267 (−1.0 %), median −1.4 %, p95 −0.7 %, p99 3.577 → 3.491 (−2.4 %); spreads 0.007 / 0.006, so small but consistent. `/Ob1` costs Windows Development about +10 % update time (Linux: +38 %). **Acted on in f0da893d:** `ConfigureRelease` now sets `AnySuitable`, as clang's `-O2` already inlines on Linux (Vc options only, so Linux's Makefiles don't change); with it, Debug / Development / Retail build with 0 warnings, Development's tests pass and Sponza closes with code 0 and 0 live. |
| A8.4 | 2026-11-10 | 5bdf3b75 | **fail** | Run without Visual Studio: a listener on `OutputDebugString`'s `DBWIN_BUFFER` (what DebugView reads, and what a debugger's Output window shows) caught the D3D11 debug layer's report. Debug `EditorHarness`, DrawLineTest, File > New Scene, close: `closed code=0`, log "0 live resources", and then 184 lines: `D3D11 WARNING: Process is terminating. Using simple reporting. Please call ReportLiveObjects() at runtime for standard reporting.`, `D3D11 WARNING: Live Producer at 0x000002A30CB508E0, Refcount: 60.`, then 182 `Live Object at 0x…, Refcount: 0.` The plain Debug `Editor.exe` on DrawLineTest, closed with no clicks, gives the same (`Live Producer … Refcount: 62`, 214 lines), so it's neither the harness nor New Scene. The device outlives `DX11Device::Shutdown`'s `mDevice->Release()` (`DX11Device.cpp:490`): about 60 references are held elsewhere. ImGui's backend takes one and returns it (`DX11ImGuiRenderer.cpp:41`). `-gfxliveobjects` (added after this) now writes the detailed report to the log: on DrawLineTest 237 objects outlive shutdown (101 `ID3D11DepthStencilView`, 31 `ID3D11Buffer`, 24 `ID3D11Texture2D`, 24 `ID3D11ShaderResourceView`, 16 `ID3D11PixelShader`, 14 `ID3D11UnorderedAccessView`, 13 `ID3D11RenderTargetView`, 7 compute shaders, 4 vertex shaders, 3 input layouts), on Sponza 280. A8.4 is now part of the Smoke check. |
| A8.4 (after fix) | 2026-11-10 | 469f87db + fix | pass | Four causes, fixed together: `DX11ConstantBuffer`'s destructor never released its buffer (27 of 31 buffers); `RenderThread::Shutdown` dropped the commands pushed since the last frame, never running or destroying them, so the 27 shader releases the scene and stages pushed while shutting down, and the handles those commands held, stayed alive; the main `RenderView`'s stage state (shadow maps, probe arrays, view targets) and the application's render target were only destroyed after the device had shut down; `Renderer` dropped its UI frame data after it. With `-gfxliveobjects`, Debug: "No D3D11 objects outlived shutdown besides the device." on DrawLineTest (was 237), Sponza (280), RenderPerfTest and RenderTest, after File > New Scene, and in the Debug Game; no "Process is terminating" report at exit on any. CI's smoke script, Debug and Development: PASS, 0 warnings. Builds 0 warnings, unit tests pass in all three. |
