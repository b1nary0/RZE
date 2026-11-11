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
  `RZE\_build\debug\ProjectData` and `RZE\_build\development\ProjectData` don't exist. Expected now: 493 test cases
  in Debug and Development, 485 in Retail. Linux runs 9 more, in `Utils\Linux` and `Rendering\OpenGL`, which Windows
  doesn't build. The counts in the checks below are Linux's from when each was written; this one is current.
- **A3. Burn.** `RZE\_build\debug\SourceAssetBurner.exe`: exit code 0, no `[Error]` line.
- **Smoke.** `driver.ps1 launch -Harness -Config debug -Scene Sponza.scene -SettleSeconds 20 -AppArgs
  '-gfxliveobjects'`, `shot`, `close`: the scene renders, `closed code=0`, the log has no `[Error]` line and ends
  "encountered 0 live resources". Shaders compile at start-up, so this is where an fxc failure shows. **A8.4:** the log
  also says "No D3D11 objects outlived shutdown besides the device."

### Harness checks for new changes

- **D13. The render command arena is 256 MB a buffer** (`MEM_ARENA_SIZE`, `Renderer.cpp`). The 64 MB version passed at
  a979e2f1 (D13 in Results); this branch raised it to 256 MB, as one load step can need more, and the merge kept
  that. 1: Sponza settled 20 s, Debug and Development: private bytes about 384 MB (2 × 192) above the D13 row's (so
  about 1970 and 1963 MB); working set about the same, as only what's written is touched. 2: the status bar reads
  `/ 256.00 MB`, the peak still about 12090 KB on Sponza. 3: Debug `Mountain.scene` (its mesh is the largest single
  load step, 28821 KB peak on Linux) loads and shows the terrain. 4: no `arena was full`, `exceeds the arena` or
  `too large to upload` line in any of these logs. 5: `closed code=0`, 0 live.
- **A15. No C4748 suppression.** `RZE_PRAGMA_OPTIMIZE_OFF/ON` (`Platform.h`) is now only `optimize("", off/on)`: A13
  saw no C4748 on x64 without the suppression, and Microsoft's current warning tables (MSVC 14.0 on) don't list it. 1:
  A1's Development build compiles `OptimizationSwitchTests.cpp` with no C4748 and passes `-tc="RZE_DISABLE*"`. 2:
  temporarily wrap `Octahedral::Decode` in `RZE_DISABLE_OPTIMIZATION` / `RZE_ENABLE_OPTIMIZATION` (as A13.3 did), build
  Development: no warning; revert and rebuild.
- **A16. RZE_DEVEL is the root switch; RZE_Config.h is gone.** `DebugConfig.h` defines `RZE_DEVEL` (Debug and
  Development), and every switch there, plus the new `RZE_DEBUG_DRAW_ENABLED` (`RenderEngine.cpp`), comes from it.
  `RZE_Config.h` (its always-1 `RZE_DEBUG`) and `EnginePCH.h`'s DebugServices include were deleted. 1: A1's three
  builds cover the deleted header and PCH include (MSVC's PCH differs from clang's). 2: A2's counts match Linux:
  436 test cases in Debug and Development, 435 in Retail. 3: Development DrawLineTest shows the grid and axis lines
  (debug lines), and Retail `Game.exe -scene DrawLineTest.scene` runs with `closed code=0`.
- **A17. The Object Browser draws only the rows in view.** `ObjectBrowserPanel::Draw` walks the tree with an
  `ImGuiListClipper` (ImGui's large-tree pattern); open parents scrolled off are `TreePush`ed by the walk, not drawn.
  Platform-neutral ImGui code, so only a smoke test, in **Debug** (ImGui's asserts: ID stack, tree push/pop balance): 1:
  RenderPerfTest, scroll the Object Browser to the bottom (last row is the last pillar) and back to the top: no assert,
  rows shaded alternately throughout. 2: DrawLineTest, drag M4 onto NeckMech, expand NeckMech: M4 shows under it,
  indented one level; scroll so NeckMech is just off the top if the panel allows, M4 stays indented; Ctrl+Z puts M4 back
  as a root. `close` gives `closed code=0`.
- **A18. OscillatorComponent, a development-only component.** New `Engine/Src/Game/World/GameObjectComponents/
  OscillatorComponent.{h,cpp}`, the whole file inside `#ifdef RZE_DEVEL`, registered in `EngineApp.cpp` the same way.
  Run `GenerateSolutions.bat` first (new files). 1: A1's three builds (MSVC compiles the empty Retail translation unit
  and the `std::fmod`/`std::sin` code). 2: tests: 443 test cases in Debug and Development (7 new, `-sf="*Oscillator*"`),
  435 in Retail. 3: Retail `Game.exe` has no `OscillatorComponent` (e.g. `findstr /m OscillatorComponent
  RZE\_build\retail\Game.exe` finds nothing). 4: Development Editor, open any scene, Add Component > OscillatorComponent
  on an object: it bobs 1 m up and down every 2 s; untick Enabled, it goes back to where it was. Ctrl+Z, close without
  saving. Also new: Optick scopes in `ObjectBrowserPanel::Draw`, `SceneViewPanel::PreRender` and `DrawObjectIcons`,
  covered by the builds.
- **A19. GameObject keeps its TransformComponent.** `GetComponent<TransformComponent>` returns a pointer kept as the
  component is added (an `if constexpr` branch in the `GameObject.h` template, with the search in its `else` so MSVC
  doesn't warn C4702 for unreachable code), `IsRoot()` reads the object's own parent, and removing a TransformComponent
  asserts and is refused. Run `GenerateSolutions.bat` first (new test file). 1: A1's three builds, no C4702 or other
  warnings. 2: 446 test cases in Debug and Development, 438 in Retail (3 new, `-sf="*GameObjectTransform*"`). 3:
  Development DrawLineTest, drag M4 onto NeckMech in the Object Browser: M4 goes under it and stays where it was in the
  world; Ctrl+Z puts it back as a root. No asserts in the log, `close` gives `closed code=0`.
- **A20. Engine.h's "include-before issue" is gone.** The comment above `#include <Events/EventHandler.h>` in
  `Engine/Src/EngineCore/Engine.h` was deleted after clang compiled `EventHandler.h` on its own and `Engine.h` with that
  include moved to the top. Check MSVC the same way: temporarily move `#include <Events/EventHandler.h>` above
  `#include <Utils/Platform/File.h>` in `Engine.h`, build Development (A1): no errors or warnings. Put the line back
  (`git checkout -- RZE/Engine/Src/EngineCore/Engine.h` after `git status --short`, then `touch` it).
- **A21. Every path written into a file is in the serialized form (`\`).** Mesh assets' material paths, material
  assets' texture paths, Game.ini's Icon and the trash's `Original` records now go through
  `Filepath::GetSerializedPath()`; `MeshAssetImporter` names a material by `Filepath::GetStem()`. 1 (before building):
  copy `RZE\ProjectData` aside. 2: build, then A3's burn, and compare with the copy: the same files and sizes; on
  Windows only `.meshasset` files should differ (their material paths had `/`; texture paths were already `\` here),
  and only by `/` becoming `\`. 3: 449 test cases in Debug and Development, 441 in Retail. 4: Development Editor on
  RenderPerfTest.scene (pillars keep their own tints: the `DefaultMaterial` overrides found their slot) and
  Sponza.scene (textured), no errors in the log, `close` gives `closed code=0`.
- **A22. Asset writers report a file they couldn't open.** `AssetWriter::Write` returns bool; the mesh and material
  writers, the burner's embedded-texture write and `GameScene::Serialize` check `File::Open`. 1: 451 test cases in
  Debug and Development, 443 in Retail (2 new: a writer whose file is blocked by a folder returns false). 2: Mark
  `RZE\ProjectData\Material\Cube\DefaultMaterial.materialasset` read-only (Properties > Read-only), run A3's burn:
  exit code 1, `[Error] MaterialAssetWriter : ... not written: it couldn't be opened for writing.` and `1 of N
  sources failed`. Clear Read-only and burn again: exit 0. 3: Development Editor, DrawLineTest.scene: change NeckMech's
  Rotation Y, Ctrl+S (the file has the new value, no `.tmp` left in `Assets\Scenes`), Ctrl+Z, Ctrl+S: `git status`
  shows the scene unchanged.
- **A23. A failed scene save stays unsaved.** `GameScene::Serialize` returns bool and takes the new path only once it's
  written; `SceneDocument::Save`/`SaveAs` mark it saved only then, and `SaveWithPrompt` passes the result on. 1: 452
  test cases in Debug and Development, 444 in Retail. 2: Development Editor, DrawLineTest.scene, with a folder made
  at `RZE\Assets\Scenes\DrawLineTest.scene.tmp`: change Rotation Y, Ctrl+S: `[Error] Couldn't save scene ...`, the
  title keeps its `*`, the file is unchanged. Close the window, choose Save in the prompt: the Editor stays open. Also
  on Windows only: delete the folder, open `DrawLineTest.scene` in another program that locks it (or set it
  read-only), Ctrl+S: `Couldn't save scene`, still `*`. Then unlock, Ctrl+S, Ctrl+Z, Ctrl+S: `git status` shows the
  scene unchanged.
- **A24. No friend between the application and the engine.** `RZE_Application::Initialize(Window&)` is public and
  calls the virtual `OnInitialize`, which `EditorApp` and `GameApp` now override instead of `Initialize`. 1: 452 test
  cases in Debug and Development, 444 in Retail (MSVC builds `EditorApp`/`GameApp` with the new override). 2:
  Development Editor, DrawLineTest.scene: the custom title bar, maximized, the Scene View draws the scene; the log has
  `RZE_Application::Initialize() called.` once. The Game: the scene draws and the window has the game's icon.
- **A25. Object Browser: create and rename in place.** Create GameObject makes `GameObject` and puts a text box over
  its row; F2 and the row menu's Rename open the same box. 1: 452 test cases in Debug and Development, 444 in Retail.
  2: Development Editor, DrawLineTest.scene: right-click empty space in the Object Browser > Create GameObject: a
  `GameObject` row with its name selected in a box; type `Crate`, Enter: renamed. Ctrl+Z gives `GameObject` back,
  Ctrl+Z again removes it. A second create while `GameObject` exists is `GameObject (1)`. Escape, or a click
  elsewhere, keeps the name; Backspace then Enter keeps the box open with a red border. Stress2500.scene: click the
  first row, scroll the panel down a long way, F2: the panel scrolls back to it with the box focused. Undo
  everything; `git status` shows the scenes unchanged.
- **A26. ScenePanel is now ObjectBrowserPanel ("Object Browser").** The files moved, so run `GenerateSolutions.bat`
  first; the build must not still list `ScenePanel.cpp`. 1: 452 test cases in Debug and Development, 444 in Retail. 2:
  Development Editor: the panel is titled "Object Browser" and View > Windows lists it. If this machine's
  `RZE\Config\imgui.ini` still names it `[Window][Scene]`, it opens floating once: dock it top-right (above Object
  Inspector) and the layout keeps it. With no `imgui.ini`, the default layout docks it there.
- **A27. Object Browser: Add GameObject button.** The panel has a toolbar (its menu bar) with an "Add GameObject" button
  at its right end, in the same place with or without the scrollbar; the rows sit below it. 1: 452 test cases in Debug
  and Development, 444 in Retail. 2: Development Editor, Stress2500.scene: click Camera, scroll the panel to the bottom:
  the toolbar stays put. Click Add GameObject: a `GameObject` row at the bottom, scrolled into view, its name in a
  focused box; type `Probe`, Enter. Ctrl+Z twice, Ctrl+Y twice: `GameObject`, gone, `GameObject`, `Probe`. Click the
  toolbar to the left of the button: `Probe` stays selected. DrawLineTest.scene: a click on the empty space under the
  rows clears the selection; a right-click there still opens Create GameObject. Undo everything and close with Don't
  Save. The rows moved down, so re-measure the Object Browser coordinates in the run-rze skill's Windows table.
- **A28. `imgui.ini` is untracked; the drivers handle there being none.** 1: after the pull (step 1 above),
  `git status --short` doesn't list `RZE/Config/imgui.ini`, and `git check-ignore RZE/Config/imgui.ini` prints it. 2:
  copy `RZE\Config\imgui.ini` to `%TEMP%\imgui.ini.mine` and delete it. `driver.ps1 launch -Harness -Scene
  DrawLineTest.scene`: the default layout (Scene View, Object Browser top right, Object Inspector below it, Content
  Browser and Log as bottom tabs). `close`: `closed code=0 (harness); imgui.ini restored`, and there's no
  `RZE\Config\imgui.ini` and no `%TEMP%\rze-driver\imgui.ini.absent` afterwards. Copy `imgui.ini.mine` back, then
  `launch` and `close` again: the file is the same as `imgui.ini.mine` (`fc /b`).
- **A29. Selected objects are outlined (OutlineRenderStage).** New files, so run `GenerateSolutions.bat` first. 1: A2's
  counts (8 tests are new: `-sf="*SelectionOutline*,*RenderView*"` runs them). The shader translation tests aren't built
  on Windows, so check the log has no shader compile error for `Pixel_OutlineMask` or `Pixel_Outline`. 2: Development
  Editor, DrawLineTest.scene: the log's `Render pipeline:` line has `DebugDrawRenderStage -> OutlineRenderStage ->
  ImGuiRenderStage`. Click M4: a yellow line about 3 px wide around its silhouette, and the Mesh Bounds checkbox stays
  off (no wire box); the slots through its stock are traced too, as on Linux. Click empty space: no line. Drag M4 onto
  NeckMech in the Object Browser, select NeckMech: both outlined. Undo that, set NeckMech's Position to the point 60% of
  the way from the editor camera (`EditorCam`) to M4, select M4: where M4 is behind NeckMech its line is faint over
  NeckMech, full elsewhere. Delete M4 from the row menu: the line goes; Ctrl+Z: it's back. Close with Don't Save. On
  DX11 the depth comparison and the R32_UINT mask are the cross-API risks: compare against Linux's look.
- **A30. Harness clicks and drags can hold modifiers (`-Mods`).** `driver.ps1 launch -Harness -Scene
  DrawLineTest.scene`, then click M4's row: `state` selection is `M4`. `click` NeckMech's row `-Mods ctrl`: the output
  ends `with ctrl held (harness)` and the selection is `M4, NeckMech`. `click` Camera's row `-Mods shift`: a range from
  NeckMech up to Camera. `-Mods ctrl,shift` is accepted as two (PowerShell's `string[]`, sent as a JSON array even with
  one entry: `-Mods ctrl` must not reach the harness as a string, which it refuses with `"mods" is an array`). With no
  session, `driver.ps1 click -X 1 -Y 1 -Mods ctrl` stops with `-Mods needs a harness session`. Close.
- **A31. Outlines are made of shapes (`RenderOutlineObject::Shape`), each edged on its own.** A new test file, so run
  `GenerateSolutions.bat` first. 1: A2's counts (7 test cases are new: `-tc="Outlined objects*"` runs them). 2: the
  log has no shader compile error for `Pixel_OutlineMask` or `Pixel_Outline` (the mask now packs a shape ID in its top
  23 bits, and the outline pass declares the mask's depth at t1). 3: A29's M4 and NeckMech checks look as they did:
  the Editor still puts the whole selection in shape 0, so nothing on screen changes yet. The mask depth read is
  checked with the next item.
- **A32. Each selected object is outlined on its own, also inside or in front of another.** 1: A2's counts (3 test
  cases are new: `-tc="*SelectionOutline*,*selected*"`). 2: `driver.ps1 launch -Harness -Scene Sponza.scene
  -SettleSeconds 20 -AppArgs '-gfxvalidation'`: click Tank's row, then Sponza's with `-Mods ctrl`. Tank has its line
  (before this change it had none): outside Tank against the floor, and on Tank's side along the green banner that's
  in front of it, the same width (about 3 px) on both. The log has no D3D11 error or warning about a resource bound
  as an input and as a depth target (the mask's depth is read at t1). 3: DrawLineTest, NeckMech's Position set to
  (12.98, 9.77, -15.79) so it's in front of M4; click M4, then NeckMech with `-Mods ctrl`: NeckMech's edge over M4 has
  a line, as on Linux. Undo the edits and close with nothing to save.
- **A33. The merge of origin's shutdown and probe work (ad7edf72) is repaired.** Its conflicts had been settled by
  taking one side: `RenderView::ReleaseStageState` and `RenderPipeline`'s `RenderWorkCounts` argument are back, on
  the renamed `m_frameBlackboard`/`m_persistentBlackboard`; `OscillatorComponent` is this branch's (Axis, Amplitude,
  Period, Phase, Origin, `RZE_DEVEL` only), registered once, and `RenderPerfTest.scene`'s two oscillators are in its
  format. 1: A1's three builds (MSVC compiles `RenderView.cpp` and `RenderPipeline.cpp` as merged). 2: A2's counts.
  3: the Smoke check, with `-gfxliveobjects`: "No D3D11 objects outlived shutdown besides the device." (the thumbnail
  view's `ReleaseStageState` is what this needs). 4: Development Editor, `RenderPerfTest.scene`: tick Enabled on
  PointLight_Shadowed_0's and Nanosuit_0's OscillatorComponent: the light swings 4 m along Z every 6 s, Nanosuit 2 m
  along X every 4 s, and the status bar's Scene updates count probe captures and shadow faces. Untick both: each is
  back where it started. Ctrl+Z twice, close with nothing to save.

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
