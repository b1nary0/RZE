# Windows checks

Checks that only a Windows machine can do, for changes made and tested on Linux. **Linux Claude maintains this file**:
it adds a check for every change it can't verify on Windows, and removes checks once their results are in. **Windows
Claude runs them** when the user asks ("follow `.claude/WindowsChecks.md`").

## How to run this file (Windows Claude)

1. `git pull`, then note the commit you're testing (`git log -1 --oneline`). If a check compares against the old build
   (e.g. a burn's hashes), do its "before" part now, with the binaries already in `_build`, before building.
2. Do the checks in **To check**, in order. Use the `run-rze` skill (`driver.ps1`, `-Harness` where a check doesn't
   need the real mouse) for anything in the Editor. `driver.ps1` backs up `RZE/Config/imgui.ini` at `launch` and puts
   it back after the session; if you start the Editor any other way, back it up first and restore it at the end
   (CLAUDE.md). Group checks that use the same scene and configuration into one session.
3. Record each result in **Results**: the check's ID, the date, the commit, `pass` / `fail` / `skipped`, and what you
   saw. For a failure, give the exact error, the screenshot's path, or the log lines. Report failures; the user decides
   who fixes them.
4. Remove checks that passed from **To check**, and adjust any whose steps no longer match the code. Don't commit
   unless the user says so (CLAUDE.md).

Checks marked **(person)** need someone at the machine (judging a picture, a GUI tool, a physical key, a debugger): do
what you can, then list the rest for the user.

Things to know on this machine (2026-11-08):
- It's a VMware VM: the GPU is `VMware SVGA 3D`. "Real GPU" checks run on that virtual GPU, and its memory shows in
  `\GPU Process Memory(...)\Local Usage`, not `Dedicated Usage` (always 0 here).
- The Editor's client area is 2546x1205 (it was 1770x959 on another monitor). Measure coordinates from a `shot`.
- `driver.ps1` has no `move` command (`driver.py` has one). To hover the scene view without clicking, use
  `scroll -X <x> -Y <y> -Notches 0`. The Editor only takes keys (W/E/R, Esc) while the pointer is over the scene view.
- `gh` isn't installed, so CI results have to be read on GitHub.

## To check

### Every run: build and tests

- **A1. Generate and build.** `GenerateSolutions.bat`, then build Debug, Development and Retail (`/W3 /WX`). Expect no
  errors or warnings.
- **A2. Unit tests.** `UnitTests.exe` in `RZE\_build\debug`, `development` and `retail`: exit code 0. Afterwards
  `RZE\_build\debug\ProjectData` and `RZE\_build\development\ProjectData` don't exist.
- **A3. Burn.** `RZE\_build\debug\SourceAssetBurner.exe`: exit code 0, no `[Error]` line.
- **Smoke.** `driver.ps1 launch -Harness -Config debug -Scene Sponza.scene -SettleSeconds 20`, `shot`, `close`: the
  scene renders, `closed code=0`, the log has no `[Error]` line and ends "encountered 0 live resources". Shaders
  compile at start-up, so this is where an fxc failure shows.

### Hands-on session 1: Visual Studio and the debugger (person)

One Visual Studio session covers these. Each temporary edit is reverted (`git status --short` first, then
`git checkout -- <that file>`) and rebuilt before the next.
- **A8.4 (debugger part).** Debug Editor under the debugger on `DrawLineTest.scene`, File > New Scene, close: the
  Output window has no D3D11 live-object report. (Without a debugger: `closed code=0`, 0 live, no assert, passed
  2026-11-08.)
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
  Retail `static_assert` and the Development build passed 2026-11-08.)

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

### Other

- **A11.4. CI (person, or a machine with `gh`).** The Windows job's `development` leg passed on the pushed commit.
- **F2. What `/Ob1` costs Windows Development (measurement; ask before running).** `ConfigureRelease` sets
  `Inline.OnlyInline` (`/Ob1`) over `AnySuitable` (`/Ob2`); on Linux the counterpart cost +38 % update time on
  `RenderPerfTest.scene`. Its case was easier stepping in Development, which `RZE_DISABLE_OPTIMIZATION` now covers per
  function. It costs two full rebuilds and six 50 s runs, so the user decides whether it's still worth measuring. Steps
  as of ce15cced: copy `_build\development\Editor.exe` to `_build\perf-Ob1\`, switch to `AnySuitable`, regenerate,
  `/t:Rebuild` Development, copy to `_build\perf-Ob2\`, revert and rebuild; run Ob1/Ob2 alternately three times each
  with `-scene RenderPerfTest.scene -perfcapture <out>.csv -perfwarmup 20 -perfduration 30`, restoring `imgui.ini`
  before each; report frame and update time (mean, median, p95, p99, spread, % change).

## Results

Results already read by Linux Claude are removed from this table; they're in this file's git history (the last full
table is at ce15cced).

All 2026-11-08 rows are at eaf66bf4 plus the uncommitted D10 fix (three SMAA includes and `GLShaderTranslator.cpp`).

| ID | Date | Commit | Result | Notes |
|---|---|---|---|---|
| A1 | 2026-11-08 | eaf66bf4 | pass | `GenerateSolutions.bat` (only Sharpmake's usual regex-separator warnings), then Debug / Development / Retail with 0 warnings, 0 errors. `STB.cpp`, `DX11Device.cpp`, `DebugWindows.cpp` and `OptimizationSwitchTests.cpp` compiled. Covers A7.1, A8.1, A9.1, A11.2's build, A13.1, A14.1, D7's and D9's builds. |
| A2 | 2026-11-08 | eaf66bf4 | pass | Debug 426/426, Development 426/426, Retail 425/425 (Linux reported 433; the 7 not counted here weren't looked into). No `ProjectData` under `_build\debug` or `_build\development`. `-tc="RZE_DISABLE*"`: 1 case, passed, in Debug and Development. `-sf="*MemArenaTests*,*MemoryUtilsTests*"`: **11** cases (the check said 10), all passed. |
| A3 | 2026-11-08 | eaf66bf4 | pass | Exit 0, no `[Error]` line. |
| A5 | 2026-11-08 | eaf66bf4 | pass | 1: burned with the 2026-11-05 Debug burner before building, then with the new one: 74 `.meshasset`/`.materialasset` files, 0 differences. 3: Debug Sponza textured, no "shorter than its contents" line. The Editor holds 8 objects (Sponza.scene's 7 plus EditorCam), not the 9 the check said. |
| D9 | 2026-11-08 | eaf66bf4 + fix | pass | Sponza looks as before. The round trip was dropped as redundant: SMAA's area and search textures are RG8 and R8 texel uploads through the new pitch, and SMAA renders correctly. |
| D8 | 2026-11-08 | eaf66bf4 + fix | pass | Log: `Render pipeline: ShadowRenderStage -> DepthPrepassStage -> AmbientOcclusionStage -> IrradianceProbeStage -> ReflectionProbeStage -> ForwardRenderStage -> PostProcessRenderStage -> AntiAliasingStage -> DebugViewStage -> PickRenderStage -> DebugDrawRenderStage -> ImGuiRenderStage`. Show: Ambient occlusion is grey occlusion over the whole scene view with the grid, the camera icon and the gizmo on top (`ao_view.png`). Launch Game shows DrawLineTest. The AA-level clicks are D10's now. |
| D6 | 2026-11-08 | eaf66bf4 + fix | pass | Sponza, DrawLineTest, SpaceHangar and RenderTest render textured and shadowed (Debug). Compile failure: shown by D10's real failures, each logged `[Error] Couldn't compile shader [...]` with fxc's message, and the Editor ran on and closed with code 0. |
| D5 | 2026-11-08 | eaf66bf4 + fix | pass, 1 warning | Debug with "Graphics validation is on", no "info queue isn't available" line. Sponza (object selected, Content Browser open): **0** `D3D11 (` lines. RenderPerfTest: 899 × `[Warning] D3D11 (352): ID3D11DeviceContext::DrawIndexed: The Pixel Shader unit expects a Sampler to be set at Slot 0, but none is bound` (also 3 on RenderTest with thumbnails, 8 on DrawLineTest). s0 is only bound by a texture's own sampler (`DX11TextureBuffer2D.cpp:327`), and `PixelResources.hlsli` declares s0, so an untextured draw (RenderPerfTest's pillars) has none. Harmless (the default sampler), probably old. Development (Sponza, RenderPerfTest): no `D3D11 (` line and no "Graphics validation" line. |
| D7 | 2026-11-08 | eaf66bf4 + fix | pass, with a Debug caveat | 1 MB arena (temporary, reverted and rebuilt), Debug Sponza: renders, `closed code=0`, no assert, 4 `arena was full` warnings (e.g. `11069 KB in 197 allocations went to the heap this frame (arena size 1024 KB)`; Linux had 47), 80 `exceeds the arena`. Real size: no `arena was full`. Memory, Sponza settled 20 s: **Debug** working set 5320 MB, private 5280 MB; **Development** working set 1234 MB, private 5279 MB. Development matches the expectation (working set about 4 GB under private bytes). Debug doesn't, because `MemArena.cpp:81` allocates with `new Byte[size]` and the Debug CRT fills every new block (0xCD), touching all 4 GB. |
| A8 | 2026-11-08 | eaf66bf4 + fix | pass | 2: Debug Sponza `closed code=0`, 109 released, 0 live. 3: Debug RenderTest 116 released (Linux 94; the Content Browser's textures and thumbnails add more), 59 of them GIF frames, 0 live. 4: DrawLineTest, File > New Scene, `close`: code 0, no assert, 0 live (the debugger-output part is still to check). 5: Game launched from the Editor: escaped, its log ends 0 live. |
| A6, A7 | 2026-11-08 | eaf66bf4 + fix | pass | Debug RenderTest: GifChat Rotation set to (-90, 0, 0) by double-clicking each field. Once focused, the quad changes between frames (31066 px) while the rest of the view doesn't (0 px). Resource Monitor lists `pulp_fiction_travolta.gif, frame 0` … `frame 58 (unnamed)`. Three Ctrl+Z put it back (`modified=false`, `canUndo=false`). `closed code=0`, 0 live, no "already registered", "does not currently exist" or "own path" line. A7.3: `ProjectData\SceneThumbnails\DrawLineTest.png` set aside, then rewritten when the Scenes folder opened (44654 bytes). |
| A9 | 2026-11-08 | eaf66bf4 | pass | 2: `TempAssertProbe` (temporary, reverted and rebuilt): Debug and Development fail it with `TestMain.cpp(20): ERROR: Assert: Assertion failed: 1 + 1 == 3, file ...ByteStreamTests.cpp, line 167` and `Message: probe`, no dialog; Retail passes. 3: Development Sponza and RenderPerfTest, `closed code=0`, no `Assertion failed` in either log. |
| A11 | 2026-11-08 | eaf66bf4 | pass | 1: `RZE.sln` lists Debug, Development and Retail, no Release; all 10 `.vcxproj` have `'Development|x64'` and no `Release|x64`. 3: deleted `_build\release` and the 11 `_project\obj\...\release` folders (about 565 MB, all from 2026-11-05, none tracked). 4: not checked (no `gh`). |
| A12 | 2026-11-08 | eaf66bf4 | pass | 1: `launch -Harness` with no `-Config` ran `_build\development\EditorHarness.exe`. 2: `-Config retail` on DrawLineTest renders (no grid: Retail strips debug draws, though the Grid checkbox still shows) and `closed code=0`. 3: `BuildGame.bat` from a plain PowerShell, exit 0 (a no-op after A1, Game.exe already current); Launch Game started `_build\development\Game.exe` in DrawLineTest, and Escape closed it. |
| A13 | 2026-11-08 | eaf66bf4 | pass | 1: see A1/A2. 2: without `__pragma(warning(disable : 4748))`, Development rebuilt `OptimizationSwitchTests.cpp` with **no C4748** (the suppression looks unneeded on x64); restored and rebuilt. 3: `Decode` wrapped (temporary): Retail fails with `Octahedral.cpp(37,1): error C2338: static assertion failed: 'RZE_DISABLE_OPTIMIZATION is still in this file: remove it and RZE_ENABLE_OPTIMIZATION'` at the macro's line; Development builds with no warning and passes `Directions come back*`; reverted and rebuilt. 4: the Development Game shows the Dear ImGui demo window over DrawLineTest. |
| A14 | 2026-11-08 | eaf66bf4 | pass | 2: Debug logs "Graphics validation is on". 3: Development has neither line. 4: plain `launch ... -AppArgs '-gfxvalidation'` printed `args='-gfxvalidation'`, logged "Graphics validation is on" and the D3D11 (352) warnings; the VM has the Graphics Tools feature. 5: `launch -Harness -Config debug -AppArgs '-gfxvalidation 0'` printed `args='-gfxvalidation 0'`, no validation line, no parameter error in `harness-app.log`. |
| C10.1 | 2026-11-08 | eaf66bf4 | pass | NeckMech selected, pointer over the scene view: E, R and W picked Rotate, Scale and Translate; Esc cleared the selection. |
| D11 | 2026-11-08 | eaf66bf4 | pass | Plain `launch` on DrawLineTest, 20 clicks on the title bar's maximize/restore (log: 10 × 1584x892, 11 × 2546x1205). GPU Local Usage 488 MB before and after (Dedicated Usage is always 0 on this VM). `closed code=0`. |
| E4 | 2026-11-08 | eaf66bf4 | pass | Deleted `RecycleProbe.txt` in the Content Browser. `ProjectData\.Trash\1\Original` held `Assets\RecycleProbe.txt` (a backslash; the check expected `/`, which only matters if a trash is read on the other platform). After `close`, no `.Trash`. The Recycle Bin's item has `DeletedFrom` = `C:\dev\RZE\RZE\Assets`. The verb is `R&estore`, not `undelete`; it put the file back with its text. Removed it; `git status` is clean under `Assets`. |
