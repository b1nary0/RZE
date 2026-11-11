# Windows checks

Checks that only a Windows machine can do, for changes made and tested on Linux. **Linux Claude maintains this file**:
it adds a check for every change it can't verify on Windows, and removes checks once their results are in. **Windows
Claude runs them** when the user asks ("follow `.claude/WindowsChecks.md`").

## How to run this file (Windows Claude)

1. **If `git ls-files RZE/Config/imgui.ini` prints the file, copy `RZE\Config\imgui.ini` to `%TEMP%` first**, and
   put it back after the pull: the pull that untracks it deletes it (or stops, if it has local changes). Then
   `git pull`, then note the commit you're testing (`git log -1 --oneline`). If a check compares against the old build
   (e.g. a burn's hashes), do its "before" part now, with the binaries already in `_build`, before building.
2. Do the checks in **To check**, in order. Use the `run-rze` skill (`driver.ps1`, `-Harness` where a check doesn't
   need the real mouse) for anything in the Editor. `driver.ps1` backs up `RZE/Config/imgui.ini` (local and untracked)
   at `launch` and puts it back after the session; if you start the Editor any other way, back it up first and restore
   it at the end (CLAUDE.md). Group checks that use the same scene and configuration into one session.
3. Record each result in **Results**: the check's ID, the date, the commit, `pass` / `fail` / `skipped`, and what you
   saw. For a failure, give the exact error, the screenshot's path, or the log lines. Report failures; the user decides
   who fixes them.
4. Remove checks that passed from **To check**, and adjust any whose steps no longer match the code. Don't commit
   unless the user says so (CLAUDE.md).

Checks marked **(person)** need someone at the machine (judging a picture, a GUI tool, a physical key, a debugger): do
what you can, then list the rest for the user.

Things to know on this machine (2026-11-10):
- The GPU is an **NVIDIA GeForce RTX 4070 Ti** (on 2026-11-08 it was a VMware VM with `VMware SVGA 3D`). Memory
  figures from before 2026-11-10 came from the VM, whose driver kept textures in system memory, so they don't compare
  with this machine's: measure a "before" here (e.g. a temporary revert) instead.
- The Editor's harness client area is 2560x1392. Measure coordinates from a `shot`.
- `driver.ps1` has no `move` command (`driver.py` has one). To hover the scene view without clicking, use
  `scroll -X <x> -Y <y> -Notches 0`. The Editor only takes keys (W/E/R, Esc) while the pointer is over the scene view.

## To check

### Every run: build and tests

- **A1. Generate and build.** `GenerateSolutions.bat`, then build Debug, Development and Retail (`/W3 /WX`). Expect no
  errors or warnings.
- **A2. Unit tests.** `UnitTests.exe` in `RZE\_build\debug`, `development` and `retail`: exit code 0. Afterwards
  `RZE\_build\debug\ProjectData` and `RZE\_build\development\ProjectData` don't exist. Expected now: 451 test cases
  in Debug and Development, 443 in Retail. Linux runs 9 more, in `Utils\Linux` and `Rendering\OpenGL`, which Windows
  doesn't build. The counts in the checks below are Linux's from when each was written; this one is current.
- **A3. Burn.** `RZE\_build\debug\SourceAssetBurner.exe`: exit code 0, no `[Error]` line.
- **Smoke.** `driver.ps1 launch -Harness -Config debug -Scene Sponza.scene -SettleSeconds 20 -AppArgs
  '-gfxliveobjects'`, `shot`, `close`: the scene renders, `closed code=0`, the log has no `[Error]` line and ends
  "encountered 0 live resources". Shaders compile at start-up, so this is where an fxc failure shows. **A8.4:** the log
  also says "No D3D11 objects outlived shutdown besides the device."

### Hands-on session 1: Visual Studio and the debugger (person)

One Visual Studio session covers these. Each temporary edit is reverted (`git status --short` first, then
`git checkout -- <that file>`) and rebuilt before the next.
- **A9.4. The Development assert box.** Temporarily add `AssertMsg(false, "probe");` at the top of
  `EditorApp::Initialize`, build Development, run `RZE\_build\development\Editor.exe` under the debugger: a box titled
  "RZE: Assertion Failed" with the expression, file and line. Retry stops in the debugger on that line, and Ignore
  carries on to the Editor. Then run it without a debugger and press Abort: the process ends.
- **A12.4.** Pick Development in the configuration list, close the solution and open it again: Development is still
  selected.
- **A13.3 (debugger part).** Temporarily wrap `Octahedral::Decode` (`Utils/Src/Utils/Math/Octahedral.cpp`, include
  `<Utils/DebugUtils/Debug.h>`) in `RZE_DISABLE_OPTIMIZATION` / `RZE_ENABLE_OPTIMIZATION`, each on its own line, and
  build Development. Break in `Decode` (`UnitTests.exe -tc="Directions come back*"`): Locals shows `x`, `y`, `z`,
  `fold` and `point` with values at every line, while a breakpoint in `Encode` shows its locals optimized away. (The
  Retail `static_assert` and the Development build passed 2026-11-08.) Development now inlines any function (`/Ob2`,
  F2), so this macro is the only way to step one through.

### Hands-on session 2: the Game (person)

`RZE\_build\debug\Game.exe`, then the Development and Retail builds.
- **C12. Keyboard and mouse separately.** Debug: F1 / F2 / F3 resize the window to 1280x720 / 1600x900 / 1920x1080;
  holding W, A, S, D flies the camera; with the pointer over ImGui's demo window, F1 still resizes; Escape quits with
  exit code 0. Repeat the resize, WASD and Escape with `RZE\_build\development\Game.exe`.
- **C11. The camera keeps the window's shape.** Drag a window edge to make it much wider, then much taller: Nyra keeps
  her proportions, never stretched.
- **C13.3.** Right after launch, hold W without touching the mouse: the camera moves straight ahead.
- **A13.4 (Retail part).** `RZE\_build\retail\Game.exe -scene DrawLineTest.scene` shows the scene with no ImGui.
  (Development showing the ImGui demo window over DrawLineTest passed 2026-11-08, from the Editor's Launch Game.)
- **A8.5.** Each Game run above ends with the newest `RZE\Logs\RZELOG_*.txt` saying "encountered 0 live resources".
- **C7 (Game part).** A Game built with an icon set in Project Settings shows that icon.

### Hands-on session 3: the Editor with real input (person)

`driver.ps1 launch -Scene DrawLineTest.scene` (not `-Harness`), then use the Editor by hand.
- **C1. Startup.** The splash shows with its status text, then the Editor appears with no white flash. **E3:** while
  the splash shows, the taskbar entry already says `RZEStudio`.
- **C2. Title bar.** Dragging the menu bar moves the window and Aero Snap works. Double-clicking it maximizes and
  restores. Hovering maximize shows the Snap Layouts flyout. Minimize, maximize and close work.
- **C3. Sizes.** Restore, then maximize: the same sizes as before the SDL port (maximized to the work area without
  covering the taskbar). Resizing from each edge and corner works.
- **C4. Keyboard.** WASD moves the editor camera. Typing into a field works, with Ctrl+A, Ctrl+C and Ctrl+V. Alt menu
  mode and Alt+Space work. Alt and F10 now reach the Editor: note whether anything reacts badly.
- **C5. Mouse.** The wheel zooms the scene view. The back and forward buttons are seen. Cursor shapes change: resize
  arrows on panel splitters and window edges, the I-beam in text fields.
- **C6. Close.** The window's close button and Alt+F4 both close it, and with unsaved changes both ask first.
- **C7. Icons.** The Alt+Tab icon is RZE's, and the icons are sharp at both sizes.
- **C8. DPI and refresh.** On a monitor scaled above 100 %, the Editor is the same size as before the port. The frame
  rate holds at the monitor's rate (59.94 shows as 59.94, not 59).
- **C9. Monitors.** With `LaunchMonitor` set in `Engine.ini`, the Editor opens on that monitor at its work area's
  top-left. `-console` puts the console beside it.
- **Camera flying (E5, C13.1, C13.2, C10.2, C10.3).** With the right button held in the scene view, hold each of W,
  A, S, D, Q and E past the key-repeat delay: smooth movement, no stop when repeat starts; R speeds up and Space slows
  down while held. Right-drag turns the camera smoothly, middle-drag orbits, Shift+middle pans, Ctrl+middle dollies,
  with no jump on any drag's first frame, nor when the pointer leaves the scene view and comes back mid-drag. Either
  drag stops a double-click focus animation part way. While right-dragging, E moves the camera up and the gizmo mode
  stays. Click the title bar's maximize button as fast as you can several times: each click toggles. With NeckMech
  selected and the pointer over the scene view, hold Q for two seconds: Local/World flips once.
- **F1. Optick.** With a Debug or Development Editor running, `Tools\Optick\Optick.exe` connects and takes a capture
  with frames in it.

### Hands-on session 4: CubeMap on DX11 (person)

- **A4.** `RenderPerfTest.scene` (8 shadowed point lights): move one light next to a pillar and look from each side.
  The pillar casts a shadow away from the light on every side, with no face missing, rotated or swapped. Then add a
  `ReflectionProbeComponent` to an object in `Sponza.scene` (don't save): the reflections are the right way up and the
  sky is on top.

## Results

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
| A13 | 2026-11-08 | eaf66bf4 | pass | 1: see A1/A2. 2: without `__pragma(warning(disable : 4748))`, Development rebuilt `OptimizationSwitchTests.cpp` with **no C4748** (the suppression looks unneeded on x64); restored and rebuilt. 3: `Decode` wrapped (temporary): Retail fails with `Octahedral.cpp(37,1): error C2338: static assertion failed: 'RZE_DISABLE_OPTIMIZATION is still in this file: remove it and RZE_ENABLE_OPTIMIZATION'` at the macro's line; Development builds with no warning and passes `Directions come back*`; reverted and rebuilt. 4: the Development Game shows the Dear ImGui demo window over DrawLineTest. |
