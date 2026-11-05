# Windows checks

Checks that only a Windows machine can do, for changes made and tested on Linux. **Linux Claude maintains this file**:
it adds a check for every change it can't verify on Windows, and removes checks once their results are in. **Windows
Claude runs them** when the user asks ("follow `.claude/WindowsChecks.md`").

## How to run this file (Windows Claude)

1. `git pull`, then note the commit you're testing (`git log -1 --oneline`).
2. Do every check in **To check**, in order. Use the `run-rze` skill (`driver.ps1`) for anything in the Editor. Back up
   `RZE/Config/imgui.ini` before the first launch and restore it at the end (CLAUDE.md).
3. Record each result in **Results**: the check's ID, the date, the commit, `pass` / `fail` / `skipped`, and what you
   saw. For a failure, give the exact error, the screenshot's path, or the log lines. Don't fix failures. Report them,
   and the user decides who fixes them.
4. Leave the checks in **To check** as they are; Linux Claude removes them once it has read the results. Don't commit
   unless the user says so (CLAUDE.md).

Checks marked **(work)** are Windows-only changes, not tests: ask the user before starting one.

## To check

### A. Build and tests (each time this file is run)

- **A1. Generate and build.** Run `GenerateSolutions.bat`, then build Debug and Release (`/W3 /WX`). Expect no errors
  or warnings.
- **A2. Unit tests.** `RZE\_build\debug\UnitTests.exe` and `RZE\_build\release\UnitTests.exe`. Expect exit code 0.
- **A3. Burn.** `RZE\_build\debug\SourceAssetBurner.exe`. Expect success and `git status` showing no change under
  `RZE\ProjectData`.
- **A4. CubeMap moved to Engine.** `CubeMap.h/.cpp` moved from `Utils/Src/Utils/Math` to `Engine/Src/Graphics`, and
  its test to `UnitTests/Src/Engine`. After A1, `Engine.vcxproj` lists `CubeMap.cpp` and `Utils.vcxproj` doesn't.
  `RZE\_build\debug\UnitTests.exe -tc="*cube*"` runs 2 test cases, both passing. Then check DX11 point-light
  shadows and reflection probes in `Sponza.scene` (the two users): shadows all around each point light, and
  reflections the right way up.
- **A5. Render thread shutdown.** `RenderThread::Shutdown` now sets `m_shuttingDown` together with the last signal,
  under the lock, and `Update()` returns whether to keep running (Known Issue #31). In Debug and in Release, run
  `driver.ps1 launch -Scene DrawLineTest.scene` then `driver.ps1 close`, 5 times each. Every `close` prints
  `closed code=0` within a few seconds. A hang (still running after the timeout) is a failure: say which config.
- **A6. Forced-include PCHs.** `StdAfx.h/.cpp` became `EnginePCH.h/.cpp` and `UtilsPCH.h/.cpp`, and no source file
  includes them any more: `BaseProject.UsePrecompiledHeader` force-includes them on win64 (`/FI` with `/Yu`). Linux
  generated these settings, so check them on a real build: after A1, `Engine.vcxproj` has
  `<ForcedIncludeFiles>EnginePCH.h</ForcedIncludeFiles>` with `<PrecompiledHeader>Use</PrecompiledHeader>`, and
  `EnginePCH.cpp` has `Create` (Utils the same). Debug and Release build with no C1010 ("unexpected end of file while
  looking for precompiled header") and no new warnings, and the build log shows the "Compiling Engine PCH" message
  once per configuration, not once per file. Build from a clean `obj` (delete `RZE\Engine\_project\obj` and
  `RZE\Utils\_project\obj` first) so no old `StdAfx.pch` is reused. Three files outside Engine and Utils used to
  include another project's `StdAfx.h` and now include only what they use: `Rendering/RenderThread.cpp`,
  `Rendering/RenderThreadCommands.cpp` and `EditorCameraComponent.cpp`. An undeclared standard name in one of them
  is a missing include there.
- **A7. Smaller PCHs.** `EnginePCH.h` and `UtilsPCH.h` lost `<codecvt>`, `<locale.h>`, `<assert.h>` and the
  `min`/`max` `#undef`s, and `UtilsPCH.h` includes the Utils math headers instead of `GLM/glm.hpp` and its `gtc`
  headers. Linux builds, but the Windows-only files (`Platform/Windows/*.cpp`, `Windowing/Windows/*.cpp`) were never
  compiled against it: an undeclared name in one of them (`std::wstring_convert`, `glm::...`, `assert`) is a missing
  include there. The build log no longer prints "Compiling Engine PCH".
- **A8. Layered PCHs.** `UtilsPCH.h` now holds std containers, Utils core (`Debug`, `File`, `Filepath`,
  `PrimitiveDefs`) and the Utils math headers, and `EnginePCH.h` includes it first instead of its own copies. Debug and
  Release build with no errors or new warnings. Note how long a clean Debug build of `Engine` takes (MSBuild's
  summary or `/clp:PerformanceSummary`), so later changes have a Windows number to compare with.
- **A9. PCHs for EditorLib, UnitTests and EditorHarness.** Each now force-includes its own PCH (`EditorLibPCH.h` in
  `Editor/Src`, `UnitTestsPCH.h`, `EditorHarnessPCH.h`), which includes `UtilsPCH.h`. After A1, `EditorLib.vcxproj`,
  `UnitTests.vcxproj` and `EditorHarness.vcxproj` each have `ForcedIncludeFiles` and `PrecompiledHeader` `Use`, with
  `Create` on their PCH's `.cpp`, and `Editor.vcxproj` has neither (it builds only `EditorMain.cpp`). These projects
  had no PCH on Windows before, so their files, the `Windows/` ones in EditorHarness included, are compiled with one
  for the first time. Debug and Release build, the tests pass, and B3 (the harness exits) still holds. Note the clean
  Debug build time of EditorLib and UnitTests next to A8's Engine number.
- **A10. Do projects compile in parallel? (a measurement, no change behind it).** On Linux, the build ran the
  projects one after another: no project's files started compiling until the projects it depends on had linked, so on
  32 cores most of them sat idle (15 s, against 8.5 s once every project compiles at once). Find out whether MSBuild
  does the same with RZE's solution. Change nothing; just measure and report.
  1. Note the machine's logical core count (`$env:NUMBER_OF_PROCESSORS`).
  2. In `Engine.vcxproj` and `EditorLib.vcxproj`, note whether the Debug configuration has
     `<MultiProcessorCompilation>true</MultiProcessorCompilation>` (`/MP`, files compiled in parallel within one
     project).
  3. Clean build: delete every `obj` folder inside a `_project` folder under `RZE` (one per project) and
     `RZE\_build\debug`, then run, timed with `Measure-Command`:
     `& $msbuild RZE\RZE.sln /p:Configuration=Debug /p:Platform=x64 /m /v:minimal /nologo
     /flp:"LogFile=$env:TEMP\rze-msbuild.log;Verbosity=normal;ShowTimestamp" /clp:PerformanceSummary`
  4. Report the wall clock, and from the log's timestamps, for each of Rendering, Engine, EditorLib and UnitTests:
     when its first file started compiling and when its `.lib` or `.exe` was written. The question is whether Engine's
     first compile starts before Rendering's `.lib` is written, and EditorLib's before Engine's. Include the
     performance summary's per-project table, and attach the lines of the log you read the times from.
- **A11. The parallel Makefile's generator stays out of the Windows build.** `RZE.sharpmake.cs` now registers a
  post-generation hook (`Make/Sharpmake/ParallelMakefile.sharpmake.cs`) that writes `RZE/Makefile` for the Linux
  build. It should do nothing for the `.sln`. In A1, `GenerateSolutions.bat` reports 0 errors and 0 warnings and prints
  no `ParallelMakefile: wrote` line, and no `RZE\Makefile` appears (`Test-Path RZE\Makefile` is `False`, unless one
  came from a Linux checkout of the same folder). The script compiles on Windows' Sharpmake run, which is its first
  test there: a C# compile error in it fails generation outright, so give its message.
- **A12. Source order in `BaseProject.PostResolve` leaves the `.vcxproj` files as they were.** `BaseProject` now
  reorders every project's resolved source files (compiled files first, then by path), so Linux object files are named
  after their source (`Foo.o`, not `Foo1.o` when `Foo.h` came first on disk). The Visual Studio generator sorts its own
  copy, so the projects it writes shouldn't change. Before pulling this commit, or in a worktree of the commit before
  it, run `GenerateSolutions.bat` and copy every `*\_project\*.vcxproj` and `*.vcxproj.filters` aside; then at this
  commit run it again and compare them (`fc /b`). Expect every file identical, and A1 to build `RZE.sln` with no error
  from the `SharpmakeProject` (it compiles these scripts against .NET Framework 4.8). List any file that differs.

### B. EditorHarness (H1: the harness beside the Editor)

- **B1. Solution.** `EditorHarness` appears in `RZE.sln`. The Editor's `.vcxproj` still has `/WHOLEARCHIVE:EditorLib.lib`
  and the windowed subsystem; EditorHarness has `/WHOLEARCHIVE` too, but is a console program.
- **B2. Talk to it.** From a console in `RZE\_build\debug`, run `EditorHarness.exe -scene DrawLineTest.scene`. Expect
  a `RZEHARNESS {"event":"ready"}` line once the scene has loaded. Type `{"id":1,"cmd":"info"}` and Enter. Expect a
  `RZEHARNESS` line with the title, the client size and a frame number.
- **B3. Exit.** Type `{"cmd":"quit"}`. The Editor closes, and the **process exits** with code 0 (`echo %ERRORLEVEL%`).
  It mustn't hang after the window closes. On Linux, a reader blocked in `std::cin` hung exit, which is why stdin is
  read with `ReadFile` (`HarnessStdinWindows.cpp`).

- **B4. Input without focus or the cursor (H2).** Run EditorHarness as in B2, click another window so it has focus,
  and keep the mouse still over that window. Then send:
  1. `{"id":2,"cmd":"click","x":2317,"y":123}` selects NeckMech;
  2. `{"id":3,"cmd":"click","x":2444,"y":783}` then `{"id":4,"cmd":"key","keys":"^a45{ENTER}"}` makes Rotation Y
     45, and `info`'s title ends with `*`;
  3. `{"id":5,"cmd":"dblclick","x":2317,"y":123}` focuses the camera on it;
  4. `{"id":6,"cmd":"scroll","x":880,"y":600,"notches":-5}` zooms out.

  Each answers `"ok":true` after it has played out. The real cursor doesn't move, and focus stays where it was.
  Coordinates are for a maximized 2560x1398 client area (`info`'s width and height); scale them for another size.
- **B5. Real input is dropped.** While B4's harness runs, click its window's M4 row with the real mouse: nothing should
  change. Send `{"cmd":"realinput","on":true}`, click M4 again, and it's selected.
- **B7. Screenshots from the back buffer (H3, DX11).** With the harness from B4 running, uncovered, send
  `{"id":7,"cmd":"shot","path":"C:\\Temp\\rze-shot.png"}` (or any writable path). It answers `"ok":true` with the
  path, width and height. Open the PNG and check:
  - it's the whole Editor, UI and all, the right way up;
  - its colours are right: red stays red, so R and B aren't swapped;
  - it matches a screenshot of the window, apart from the status bar's frame time.

  Then cover the window with another one and send `shot` again: the PNG still shows the Editor. This is
  `DX11Device::ReadBackBuffer`, which copies swap-chain buffer 0 before Present (FLIP_DISCARD). It was written on
  Linux without being compiled for Windows, so it's the riskiest new code. If it fails, give the error text and any
  D3D debug-layer messages. A path that can't be written must answer `"ok":false` with an error; the harness must
  keep running.
- **B8. State queries (H4).** In a fresh harness on DrawLineTest:
  1. `{"id":1,"cmd":"state"}` lists the objects (Camera, DirectionalLight, NeckMech, M4, and EditorCam with
     `"saved":false`), `"modified":false` and an empty selection.
  2. Do B4's steps 1 and 2. `state` then shows `"selection":["NeckMech"]`, `"modified":true` and `"canUndo":true`.
     `{"id":2,"cmd":"object","name":"NeckMech"}` shows `"rotation":[0,45,0]` and the components `RenderComponent`
     and `TransformComponent`, with no `class ` in front (TypeNames).
  3. `{"id":3,"cmd":"key","keys":"^z"}`. The rotation is back to `[0,0,0]`, with `"atSavePoint":true`.
- **B6. Known limit, to confirm.** ImGui's SDL backend reads the real global mouse position when the app is focused
  and the real pointer is over no SDL window (Windows and X11 only; never on Wayland). So focus the harness's window,
  move the real mouse off it, and send `click`. Note whether the click lands where the harness put it, or where the
  real mouse is.

### C. Window and input (Phase 3: SDL replaced Win32Window)

- **C1. Startup.** The splash shows with its status text, then the Editor appears with no white flash (it's cloaked
  until the first frame).
- **C2. Title bar.**
  - Dragging the menu bar moves the window, and Aero Snap works.
  - Double-clicking it maximizes and restores.
  - Hovering the maximize button shows the **Snap Layouts** flyout.
  - The minimize, maximize and close buttons work.
- **C3. Sizes.** Restore from maximized, then maximize again. Expect the same sizes as before the port (the Editor's
  default size, and maximized to the work area without covering the taskbar). Resizing from each edge and corner
  works.
- **C4. Keyboard.**
  - WASD moves the editor camera.
  - Typing into a field works, including Ctrl+A, Ctrl+C and Ctrl+V.
  - **Alt menu mode and Alt+Space** still work (an SDL hint keeps them).
  - Behaviour change: Alt and F10 now reach the Editor (Win32 used to swallow them). Note whether anything reacts
    badly.
- **C5. Mouse.**
  - The wheel zooms the scene view.
  - The back and forward buttons are seen.
  - Cursor shapes change: resize arrows on panel splitters and window edges, the I-beam in text fields.
- **C6. Close.** The close button, Alt+F4 and `driver.ps1 close` all close it. With unsaved changes they ask first.
  `close` reports `closed code=0`.
- **C7. Icons.** The taskbar and Alt+Tab icons are RZE's, and sharp at both sizes. A Game built with an icon set in
  Project Settings shows that icon.
- **C8. DPI and refresh.**
  - On a monitor scaled above 100 %, the Editor is the same size as before the port (it's DPI-unaware).
  - The frame rate holds at the monitor's rate. Q28 made refresh rates exact, e.g. 59.94, not 59.
- **C9. Monitors.** With `LaunchMonitor` set in `Engine.ini`, the Editor opens on that monitor at its work area's
  top-left. `-console` puts the console beside it.

### D. Rendering on DX11 (Phase 4: the RHI refactor moved every DX11 call)

- **D1. Scenes.** Screenshot Sponza, DrawLineTest, SpaceHangar and RenderTest. Compare with how they looked before the
  port. Expected:
  - Sponza: shadows under the arches;
  - SpaceHangar: the spaceship present (its scene reference was renamed to `Spaceship.meshasset`);
  - DrawLineTest: the M4 present (`M4A1.meshasset`).
  Note anything that differs: GTAO, reflections, FXAA, exposure.
- **D2. Picking and gizmos.**
  - Clicking an object in the scene view selects it.
  - Dragging the translate gizmo moves it.
  - Mesh Bounds draws a box.
- **D3. ImGui.**
  - The scene view image is the right way up.
  - Content Browser thumbnails regenerate (delete one from `RZE\ProjectData\SceneThumbnails` or `MeshThumbnails`)
    and are the right way up.
- **D4. Game UI.** Launch the Game. Its ImGui demo window is the right way up, with its title bar on top. On Linux this
  needed a GL-only fix; DX11 should already be right.

### E. Editor features

- **E1. Component order.** Open DrawLineTest, make no change, then Save Scene (Ctrl+S). `git diff
  RZE/Assets/Scenes/DrawLineTest.scene` must be empty: components load and save in the file's order, and separators
  stay `\\`. Revert if it isn't, and report the diff.
- **E2. Inspector.** Selecting an object shows `[ TransformComponent ]` and `[ RenderComponent ]` headings (TypeNames:
  Windows strips "class ").
- **E3. Title.** The Editor's taskbar entry says `RZEStudio` from the start, then `RZEStudio - <scene>`. The Game's
  says Engine.ini's `WindowTitle`.
- **E4. Content Browser.**
  - The right-click menu says **Show in Explorer**, and it opens Explorer with the file selected.
  - Delete's tooltip says **Recycle Bin**.
  - A file deleted there goes to the Recycle Bin once the Editor closes. Use a copy of a file made for the test,
    then remove it from the Recycle Bin.
  - A file copied into `Assets` from Explorer appears in the Content Browser within a few seconds (folder watcher).
- **E5. Build and Launch Game.** File > Launch Game... builds through `BuildGame.bat` (its output in the Log panel's
  Build channel), then starts the Game in the open scene.

### F. Profiling

- **F1. Optick.** With a Debug or Release Editor running, `Tools\Optick\Optick.exe` connects (port 31318) and takes
  a capture with frames in it.

### G. CI

- **G1. Windows.yml.** Its actions were updated: runner `windows-2022`, .NET 8, `upload-artifact@v4`. Pushes to this
  branch don't start it (it runs on pushes and PRs to `master` and `Sandbox` only), so run it by hand with
  `gh workflow run Windows.yml --ref look-what-claude-can-do`, if the user agrees, and report the result. It runs this
  branch's copy of the workflow, which also builds the new Sharpmake hook (A11). Known failure: the runner has no
  VS 2026 (v145 toolset), which `RZE.sharpmake.cs` targets since the Sharpmake 1.9.1 upgrade, so `Windows.yml` needs
  a runner image with VS 2026 first.

### H. Windows-only work (ask the user before starting)

- **H1 (work). driver.ps1: back up imgui.ini.** `driver.py` copies `RZE/Config/imgui.ini` aside at `launch`, and puts
  it back after `close`, `quit` or a dead session (d7f51a6f). `driver.ps1` should do the same, with
  `-KeepImguiIni` to opt out, and SKILL.md updated.

- **H2 (work). driver.ps1: a harness mode.** `driver.py launch --harness` (H5) runs EditorHarness, sends its
  commands as JSON lines on stdin, and reads `RZEHARNESS {json}` answers from stdout, matched by `id`. It waits for
  `"maximized":true` from `info` before returning. `driver.ps1` should get the same `-Harness` switch, routing
  click, dblclick, drag, scroll, key, shot, info and close through it, and add state, object and realinput. Do B2
  to B8 first, so the harness itself is known to work on Windows.

## Results

| ID | Date | Commit | Result | Notes |
|---|---|---|---|---|
| A1 | 2026-11-05 | b9d9ba3e + PathGetRelative fix | pass | VS 2026 (v145), SDK 10.0.26100, after the Sharpmake 1.9.1 upgrade (1c7295d7). Generation 0 errors, 0 warnings. Debug and Release build with exit code 0, no C++ errors or warnings. Before the fix, the solution build failed only on `SharpmakeProject_Win64.csproj` (A11). |
| A2 | 2026-11-05 | b9d9ba3e + PathGetRelative fix | pass after fix | Debug: 365/365 passed. Release: 364/365 at first. `OctahedralTests.cpp:26` "Directions come back from the octahedral square as they went in": `CHECK( 0.999978 == Approx( 1 ) )` (epsilon 1e-5). The generated compiler options match the old Sharpmake's apart from the toolset and SDK, so this comes from v145's Release code. Cause: the test's input direction was 0.999978 long (Decode's output was exactly 1), from v145's optimized float sin and cos at angles up to ~2400 rad. Fixed in the test, which now normalizes its input: Debug and Release 365/365. |
| A11 | 2026-11-05 | b9d9ba3e + PathGetRelative fix | pass after fix | It failed first: `ParallelMakefile.sharpmake.cs(75,31): error CS0117: 'Path' does not contain a definition for 'GetRelativePath'` in `SharpmakeProject_Win64.csproj`, which targets .NET Framework 4.8 (generation itself ran fine on .NET 8). Fixed with `Util.PathGetRelative`. Now: no `ParallelMakefile: wrote` line, and `Test-Path RZE\Makefile` is `False`. **Linux:** run `GenerateSolutions.sh` and confirm `RZE/Makefile` is unchanged. |
| A3 | 2026-11-05 | 155aad06 | pass | `SourceAssetBurner.exe` (Debug): exit 0 in 13 s, no `[Error]` lines, 80 files written. `RZE\ProjectData` is gitignored with nothing tracked, so `git status` there can't show a change: the check as written is vacuous. |
| A4 | 2026-11-05 | 155aad06 | pass (build part) | `Engine_Win64.vcxproj` lists `CubeMap.cpp` and `Utils_Win64.vcxproj` doesn't. `UnitTests.exe -tc="*cube*"`: 2 test cases, both passing, in Debug and Release. The Sponza point-light shadow and reflection probe check is still to do (P2). |
| A6 | 2026-11-05 | 155aad06 | pass | Engine, Utils: `ForcedIncludeFiles` `EnginePCH.h` / `UtilsPCH.h`, `PrecompiledHeader` `Use`, `Create` on `EnginePCH.cpp` / `UtilsPCH.cpp`. With their `obj` folders deleted (no `StdAfx.pch` was there; this machine had never built the old layout), Debug and Release build with exit 0, 0 errors, 0 warnings, no C1010; `/Yc` runs once per project per configuration. No "Compiling Engine PCH" message, as A7 expects. RenderThread.cpp, RenderThreadCommands.cpp and EditorCameraComponent.cpp compile. Every project builds with `/fp:fast`, Debug included. |
| A7 | 2026-11-05 | 155aad06 | pass | Every Windows-only file (`Platform/Windows`, `Windowing/Windows`, EditorHarness `Windows/`) compiles against the smaller PCHs in Debug and Release: no undeclared names. The build log has no "Compiling Engine PCH". |
| A9 | 2026-11-05 | 155aad06 | pass (build part) | EditorLib, UnitTests, EditorHarness: `ForcedIncludeFiles` with their own PCH, `Use`, `Create` on the PCH's `.cpp`. `Editor_Win64.vcxproj`: no forced include, `NotUsing`. Debug and Release build; tests 365/365 in both. B3 still to do (P2); the build times belong to A8 (not done). |
| B1 | 2026-11-05 | 155aad06 | pass | `EditorHarness` is in `RZE.sln`. Editor: `/ENTRY:mainCRTStartup /WHOLEARCHIVE:EditorLib.lib`, SubSystem `Windows`. EditorHarness: `/WHOLEARCHIVE:EditorLib.lib`, SubSystem `Console`. |
| B2 | 2026-11-05 | f7b3ca3e + BOM fix | pass | Debug and Release, `-scene DrawLineTest.scene` from `_build\<config>`: `ready` after 4–5 s; `info` answers with the title, 1770x959, `"maximized":true` and the frame. At first, the first command answered `Not a command`: Windows PowerShell 5.1's `Process.Start` writes a UTF-8 BOM to a redirected stdin. `HarnessChannel` now drops a BOM before the first line; with the BOM still sent, the first `info` answers in 0.1 s. |
| B3 | 2026-11-05 | f7b3ca3e + BOM fix | pass | After `{"cmd":"quit"}` the process exits with code 0 within 1.5 s, in Debug and Release (4 runs). |
| B7 | 2026-11-05 | f7b3ca3e + BOM fix | pass | `shot` answers `"ok":true`, 1770x959, in ~0.5 s (Debug and Release). The PNG is the whole Editor, the right way up, with the grid's X axis red and Z blue (R and B not swapped). Covered by a maximized Notepad, `shot` still shows the Editor (its title bar buttons dimmed: not active). A path in a missing folder works (`create_directories`); a path under an existing *file* answers `"ok":false` ("Couldn't write ...: Cannot create a file when that file already exists.") and the harness keeps answering. |
| A5 | 2026-11-05 | f7b3ca3e + BOM fix | pass | `driver.ps1 launch -Scene DrawLineTest.scene` then `close`, 5 times in Debug (1.5–1.6 s each) and 5 in Release (1.3–1.4 s): all `closed code=0`, no hang. |
| C6 | 2026-11-05 | f7b3ca3e + BOM fix | pass (driver close only) | After Rotation Y = 45 (title ends with `*`), `driver.ps1 close` stops at "Unsaved Changes: Assets\Scenes\DrawLineTest.scene has unsaved changes. Save them before exiting? Save / Don't Save / Cancel". Don't Save exits it, and the scene file is unchanged. The close button and Alt+F4 are still to check (by hand). |
| E2 | 2026-11-05 | f7b3ca3e + BOM fix | pass | Selecting M4 shows `[ TransformComponent ]` and `[ RenderComponent ]`. |
| D1 | 2026-11-05 | f7b3ca3e + BOM fix | pass (no before images) | Sponza: textured, the colonnade shadowed under the arches, sunlit courtyard. SpaceHangar: the spaceship is there. DrawLineTest: the M4 is there. RenderTest: Nyra, the camera frustum and the light icon. No screenshots from before the port were at hand to compare GTAO, reflections, FXAA or exposure against. |
| A4 | 2026-11-05 | f7b3ca3e + BOM fix | runtime part inconclusive | Sponza.scene has no point lights or reflection probes (one LightComponent, directional), and no scene has a ReflectionProbeComponent. RenderPerfTest has 8 shadowed point lights; focused on PointLight_Shadowed_0, the pillars facing it are lit, but whether shadows fall on every side can't be judged from screenshots. Needs a person, and a probe added by hand for the reflection part. |
| B4 | 2026-11-05 | 3cc252f6 | pass | 1770x959 client, so coordinates measured from a shot: NeckMech row (1526,123), Rotation Y (1654,563), scroll at (730,500). With Notepad focused: click, key `^a45{ENTER}`, dblclick and scroll each answer `"ok":true`. Rotation Y = 45 and the title ends with `*`; the dblclick focuses the camera on NeckMech; scroll -5 zooms out (both seen in harness shots). The real cursor stayed at 1503,145 and Notepad kept the focus throughout. |
| B8 | 2026-11-05 | 3cc252f6 | pass | Fresh: objects Camera, DirectionalLight, NeckMech, M4 and EditorCam (`"saved":false`), `modified:false`, empty selection. After B4 1–2: `selection:["NeckMech"]`, `modified:true`, `canUndo:true`; `object NeckMech`: `rotation:[0,45,0]`, components `RenderComponent`, `TransformComponent` (no `class `). After `^z`: `rotation:[0,0,0]`, `atSavePoint:true`. |
| B5 | 2026-11-05 | 3cc252f6 | pass after fix | Real click on M4 while blocked: nothing changes. After `realinput on`, a real click on M4 selects it. At first, a real click right after `realinput on` with no real move landed at the harness's last injected position: here (730,500), over the gizmo's Y arrow, making a zero-length transform edit. The UI only learns the mouse position from move events, and the real ones were dropped while blocked. Fixed in `HarnessInput::SetRealInputBlocked`: on unblocking, if the real cursor is over the window (`SDL_GetMouseFocus`), a move to it (`SDL_GetMouseState`) is pushed. Retest (Debug, Release): the harness's mouse left on the gizmo, the real cursor on M4, `realinput on`, a real click with no move: `selection=M4 modified=False`. **Linux:** under `driver.py --harness`, `realinput on` then a real click without moving the mouse lands under the cursor. |
| B6 | 2026-11-05 | 3cc252f6 | limit not seen | Harness window focused, real cursor moved to (1000,990) over the taskbar (off every SDL window); harness `click` on NeckMech selects NeckMech: it landed where the harness put it. |
| D2 | 2026-11-05 | 3cc252f6 | pass after fix | It failed first: clicks on the M4 in the scene view mostly selected nothing (real input and harness, Debug and Release). A 20 px harness grid sweep put the pick hits about 40 px above the gun as drawn; object icons (the light's sun above its direction line) and the gizmo (above the mesh-bounds centre) were 40 px high too. Cause, logged: `SceneViewPanel` cached the panel's position only when x **and** y both changed (`&&`, since eeb59cd6, 2022). Docked at x = 0, the panel moved only down, under the 40 px title bar, so `m_position` stayed (0,0) while the window was at (0,40). Picks, marquee, icons, mesh drops and the gizmo's rect all use it. Not DX11-specific: plain ImGui screen coordinates, no texture origin or flip. Fix: the position is taken every frame. After: the grid sweep's hits match the gun's outline, identical in Debug and Release (exactly 40 px lower than before); real clicks at (930,480) and (860,530) select M4; the gizmo sits in the bounds box's centre; dragging its X arrow moves the M4 (Position X 24.297 to 16.969) and Ctrl+Z restores it; RenderTest's sun and camera icons sit on their lines. Mesh Bounds draws a box. **Linux:** the default layout docks the Scene View at x = 0 too, so it most likely had the same offset: rerun the grid sweep (or click a mesh's edge) at 2560x1398. |
| D3 | 2026-11-05 | 71932db6 + D2 fix | pass | `ProjectData\SceneThumbnails\DrawLineTest.png` deleted: the Editor rendered it again at startup ("Rendering thumbnails – Done"), byte-identical to the deleted one (MD5 47688EC2…). The Content Browser's Scenes thumbnails are the right way up and not mirrored (also SpaceHangar.png opened directly). There's no MeshThumbnails folder on disk, so mesh thumbnails weren't checked separately. |
| E1 | 2026-11-05 | 71932db6 + D2 fix | pass | DrawLineTest opened, no change, Ctrl+S: the file was rewritten (mtime 02:14:01 to 13:42:14) and `git diff` on it is empty. |
| E3 | 2026-11-05 | 71932db6 + D2 fix | pass (taskbar after load) | The Editor's taskbar entry reads `RZEStudio - Assets\Scer…`; the Game's reads `Game Title` (Engine.ini `WindowTitle`). The title during the splash wasn't caught. |
| E4 | 2026-11-05 | 71932db6 + D2 fix | pass | Folder watcher: a file copied into `Assets\2D` from outside showed in the Content Browser within 4 s. Right-click menu: Show in Explorer, Copy Path, Rename (F2), Delete. Show in Explorer opened Explorer in `Assets\2D`; the file was selected the second time ("1 item selected"), not the first, with the same `explorer /select` command, which selects every time when run by hand: most likely an Explorer race on a fresh window. Delete's tooltip: "…it goes to the Recycle Bin once it drops out of the undo history"; a confirmation names the file. Delete moved it to `ProjectData\.Trash\1\Item\`; on close (`closed code=0`) it went to the Recycle Bin, then I removed it from there. Finding: the Recycle Bin records its original location as `ProjectData\.Trash\1\Item`, so Restore from the Recycle Bin puts the file in the trash folder, not back in `Assets\2D` (likely the same on Linux). |
| E5 | 2026-11-05 | 71932db6 + D2 fix | pass | File > Launch Game… on DrawLineTest: the Log's Build channel shows the msbuild output (Rendering, Externals, Engine, Utils, Game.exe) and "Build and Launch Game succeeded."; the Game started in that scene within ~30 s. It closed on WM_CLOSE. |
| D4 | 2026-11-05 | 71932db6 + D2 fix | pass | The Game's Dear ImGui Demo window is the right way up, its title bar on top; NeckMech and the M4 render. |
| C7 | 2026-11-05 | 71932db6 + D2 fix | pass (taskbar) | Taskbar: the Editor and the Game show RZE's icon. EditorHarness shows the generic one, because it doesn't embed `Editor.rc` (the Editor and the Game do): accepted, it's a test tool. Alt+Tab and sharpness at both sizes are left for a person. |
| F1 | 2026-11-05 | 71932db6 + D2 fix | partial | The Debug Editor listens on 31318 and a TCP connect succeeds; `Tools\Optick\Optick.exe` is there. Taking a capture needs Optick's GUI: left for a person. |
| H1 | 2026-11-05 | 53990f24 | done | driver.ps1 backs up `RZE\Config\imgui.ini` at `launch` (`%TEMP%\rze-driver\imgui.ini.backup`) and puts it back after `close` and `quit`; `-KeepImguiIni` opts out. Tested: a normal session, a close stopped at the prompt (backup kept until the Editor exits), a dead session's leftover (restored by the next launch), and `-KeepImguiIni`. **Linux:** with `--keep-imgui-ini`, driver.py's `close` still restores a backup an earlier dead session left, overwriting the kept layout; driver.ps1 restores such a leftover at `launch` instead. |
| H2 | 2026-11-05 | f57f68a9 | done | `driver.ps1 launch -Harness`: a hidden helper holds EditorHarness's pipes; calls reach it through the named pipe `rze-driver-harness`. Every command goes through the harness, plus `state`, `object`, `realinput`; `scroll` added for both modes. `close` stops at the Unsaved Changes prompt, which is answered with `click` on Don't Save. B2–B8 rerun through it in Debug and Release: all pass (real cursor and focus untouched in B4, B5's real click lands after `realinput -On on` with no move, covered `shot`, prompt and Don't Save, exit code 0, imgui.ini restored). |
| A8 | 2026-11-05 | f57f68a9 | measured | 16 logical cores, VS 2026 (v145), `/MP` on in all three. Each project's own clean Debug build, alone (`BuildProjectReferences=false`, its references already built, its `obj\…\debug` deleted first), two runs each: **Engine 6.4 / 6.2 s** (108 objects), **EditorLib 6.6 / 6.6 s** (66), **UnitTests 8.7 / 8.7 s** (71). For context, the whole solution with those three `obj` folders deleted: 18.5 s and 15.3 s wall clock (MSBuild's per-project times there include waiting on references, so they aren't own times). |
| A10 | 2026-11-05 | 416a87a4 | measured | 16 logical cores; `/MP` on (A8). Clean Debug build (every `obj\…\debug` and `RZE\_build\debug` deleted), `/m`, timestamped log: **17.7 s** wall clock. Seconds from the log's first line, as first ClCompile / Lib or Link starts / project done: Externals 0.29 / 2.38 / 2.95; Rendering 0.30 / 3.49 / 4.15; Utils 0.30 / 4.60 / 5.79; Engine 0.32 / 7.87 / 8.18; EditorLib 0.34 / 7.52 / 7.85; SourceAssetBurner 8.29 / 10.24 / 12.57; Game 8.29 / 11.34 / 14.47; Editor 8.29 / 11.38 / 15.39; EditorHarness 8.32 / 12.21 / 15.77; UnitTests 8.32 / 13.52 / 16.38. **Answer: the static libraries compile in parallel from the start:** Engine's first compile (0.32 s) comes before Rendering's `.lib` (Lib at 3.49 s), and EditorLib's (0.34 s) before Engine's (Lib at 7.87 s). **The executables don't:** none of the five starts compiling its own files until every library it references is done (8.3 s), so about 8 s of the 17.7 s is libraries, then exes. Log lines (ids: Rendering 11, EditorLib 4, Engine 6, UnitTests 7): `14:09:08.018 11>ClCompile:` · `14:09:08.040 6>ClCompile:` · `14:09:08.059 4>ClCompile:` · `14:09:11.211 11>Lib:` → `Rendering.lib`, done 14:09:11.868 · `14:09:15.240 4>Lib:` → `EditorLib.lib`, done 14:09:15.571 · `14:09:15.595 6>Lib:` → `Engine.lib`, done 14:09:15.898 · `14:09:16.037 7>ClCompile:` · `14:09:21.244 7>Link:` → `UnitTests.exe`, done 14:09:24.106. Side finding: an empty `RZE\_build\debug\ProjectData\` tree existed before the clean (no files); something creates it relative to the working directory. |
| A12 | 2026-11-05 | 206af2c0 | pass | Same checkout rather than a worktree, so the absolute paths match: `GenerateSolutions.bat` with c964afd8's `BaseProject.sharpmake.cs` put back by hand, the 20 `*_Win64.vcxproj` / `.vcxproj.filters` under `*\_project` copied aside, then the new file restored and generated again. Both runs: 0 errors, 0 warnings, "0 generated, 21 up-to-date". All 20 files byte-identical (`cmp`). No `ParallelMakefile: wrote` line and no `RZE\Makefile` (A11 still holds). |
| A1 | 2026-11-05 | 206af2c0 | pass | `RZE.sln` Debug and Release, `/m`: exit 0, 0 errors, 0 warnings. `SharpmakeProject_Win64` ran `csc` against .NET Framework v4.8 with `BaseProject.sharpmake.cs` (the new `System.Linq` / `List<string>` code) in both configurations. The C++ projects were up to date: no source or `.vcxproj` changed (A12). |
| A2 | 2026-11-05 | 206af2c0 | pass | Debug and Release `UnitTests.exe`: 365/365, exit 0. |
