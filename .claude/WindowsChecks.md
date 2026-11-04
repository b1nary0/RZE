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

- **G1. Windows.yml.** Its actions were updated: runner `windows-2022`, .NET 8, `upload-artifact@v4`. Run it on this
  branch with `gh workflow run Windows.yml --ref look-what-claude-can-do-linux-port`, if the user agrees, and report
  the result. Known risk: the runner may lack VS2017's v141 toolset or SDK 10.0.18362, which `RZE.sharpmake.cs` pins.

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
