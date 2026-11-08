# Windows checks

Checks that only a Windows machine can do, for changes made and tested on Linux. **Linux Claude maintains this file**:
it adds a check for every change it can't verify on Windows, and removes checks once their results are in. **Windows
Claude runs them** when the user asks ("follow `.claude/WindowsChecks.md`").

## How to run this file (Windows Claude)

1. `git pull`, then note the commit you're testing (`git log -1 --oneline`).
2. Do every check in **To check**, in order. Use the `run-rze` skill (`driver.ps1`, `-Harness` where a check doesn't
   need the real mouse) for anything in the Editor. `driver.ps1` backs up `RZE/Config/imgui.ini` at `launch` and puts
   it back after the session; if you start the Editor any other way, back it up first and restore it at the end
   (CLAUDE.md).
3. Record each result in **Results**: the check's ID, the date, the commit, `pass` / `fail` / `skipped`, and what you
   saw. For a failure, give the exact error, the screenshot's path, or the log lines. Don't fix failures. Report them,
   and the user decides who fixes them.
4. Leave the checks in **To check** as they are; Linux Claude removes them once it has read the results. Don't commit
   unless the user says so (CLAUDE.md).

Checks marked **(work)** are Windows-only changes, not tests: ask the user before starting one. Checks marked
**(person)** need someone at the machine (judging a picture, a GUI tool, a physical key): do what you can, then list
the rest for the user.

## To check

### A. Build and tests (each time this file is run)

- **A1. Generate and build.** Run `GenerateSolutions.bat`, then build Debug and Development (`/W3 /WX`; Development was
  called Release before the commit after 139bfcb1). Expect no errors or warnings. **Since the commit after 3c2a07d8:**
  every component class is `final`, and `GameObjectComponent<T>`'s constructor has `static_assert(std::is_final_v<T>)`;
  expect MSVC to accept both, with no new warnings. **Since the commit after 293171bb:**
  `Engine/Src/EngineCore/Platform/Memory/` (Memory.h, BlockAllocator.h/.cpp) is deleted, so `GenerateSolutions.bat` must
  run before building, or the old `.vcxproj` still lists the missing files. The math vectors lost `ToString` and
  StringUtils lost `FormatString`; their includes were left as they were, so nothing should depend on a header MSVC no
  longer pulls in. Report any "cannot open include file" or missing `std::` name.
- **A2. Unit tests.** `RZE\_build\debug\UnitTests.exe` and `RZE\_build\development\UnitTests.exe`. Expect exit code 0.
  **Since the commit after f5faea2e:** the tests that write into the editor's trash and thumbnail folders remove the
  empty folders they made (`RemoveEmptyFolders`, new `UnitTests/Src/TestFolder.cpp`), so after both runs
  `RZE\_build\debug\ProjectData` and `RZE\_build\development\ProjectData` don't exist (A10 found an empty one). A tree
  left by older runs goes in the same run, unless something in it isn't empty: list what's there if so.
- **A3. Burn.** `RZE\_build\debug\SourceAssetBurner.exe`. Expect exit code 0 (it exits with 1 when an import fails)
  and no `[Error]` line in its output. `RZE\ProjectData` is gitignored, so `git status` can't show what it wrote.
- **A4. CubeMap's face layout on DX11 (person).** The build part passed (2026-11-05). Left: the two users of
  `CubeMap`, on a real GPU.
  - **Point-light shadows:** `RenderPerfTest.scene` (8 shadowed point lights; Sponza has none). Move one light next to
    a pillar and look from each side: the pillar casts a shadow away from the light on every side, with no face
    missing, rotated or swapped.
  - **Reflection probes:** no scene has one. Add a `ReflectionProbeComponent` to an object in `Sponza.scene` by hand
    and don't save: the reflections are the right way up and the sky is on top.

- **A5. Asset readers fail cleanly; burns are byte-identical (the commit after 10407443).** `ByteStream` fails
  instead of reading or writing past its end, `ByteStreamUtils` checks sizes before allocating, and the importers
  reject a truncated or corrupt `.meshasset`/`.materialasset`. On Linux, 72 burned assets hashed the same before and
  after, and the new tests passed in Debug and Release. MSVC's `size_t`, padding and `EBlendMode` size are what could
  differ:
  1. Before building the new commit, burn with the old one and hash: `Get-ChildItem RZE\ProjectData -Recurse -Include
     *.meshasset,*.materialasset | Get-FileHash | Sort-Object Path | Out-File $env:TEMP\hash_before.txt`. Burn again
     with the new build (A3) and hash to `hash_after.txt`; expect `Compare-Object` to find no difference.
  2. A2 covers the new tests (`ByteStreamTests`, `MeshAssetTests`, the material truncation tests), in Development too.
  3. `driver.ps1 launch -Harness -Scene Sponza.scene -SettleSeconds 20`: expect 9 objects, textured, and no
     "shorter than its contents" lines in the log.
- **A6. Resource keys are relative paths (the commit after 6f246ec9).** A resource's key is now its relative path,
  extension kept, which on Windows has `\` separators; handles hold no key. On Linux, DrawLineTest, RenderTest and
  Sponza released every resource they made (39, 94 and 109) with no "already registered" or "does not currently
  exist" lines.
  1. A2 covers the new `ResourceHandlerTests` cases, including a `Make` ID with a `/` in it.
  2. `driver.ps1 launch -Harness -Scene RenderTest.scene -SettleSeconds 10`, then `close`: expect GifChat's frames to
     play, `closed code=0`, and the log to end with "encountered 0 live resources" and no "isn't stored under its own
     path" assert.
- **A7. Unnamed resources; stb compiled in Externals (the commit after 9d4792e9).** `ResourceHandler::Make` became
  `AddUnnamedResource`; GifChat decodes with stb's own `stbi_load_gif_from_memory`; stb_image and stb_image_write are
  compiled once, in `Externals/Src/STB/STB.cpp`, instead of inside GifChatComponent.cpp and TextureFile.cpp. Linux
  hides Externals' warnings (`-w`); Windows builds it with the engine's MSVC flags, so:
  1. A1 must build `Externals` with no warnings from `STB.cpp` (stb compiled under the same flags in Engine before).
  2. `driver.ps1 launch -Harness -Scene RenderTest.scene -SettleSeconds 10`: set GifChat's Rotation to (-90, 0, 0) by
     double-clicking each field, and expect the GIF to play on the quad. View > Windows > Resource Monitor lists 59
     "pulp_fiction_travolta.gif, frame N (unnamed)" rows. Undo the three edits, then `close`: `closed code=0` and
     "encountered 0 live resources".
  3. Saving a thumbnail still writes a PNG (stb_image_write moved): any Content Browser thumbnail regenerates.
- **A8. One teardown path; components get Uninitialize (the commit after 602c8030).** `GameScene::ShutDown` is gone:
  shutdown calls `Unload`, so every object leaves the scene (destroying render objects, lights and probes) before the
  renderer shuts down, and `Engine.cpp` asserts the main render scene is empty. Components have `Uninitialize`;
  CameraComponent releases its preview render target there and GifChat its frames. DX11 releases GPU objects in a
  different order than before, so:
  1. A1/A2 as usual (MSVC `/WX` on the new pure virtual and the structured binding in `GameScene::AddGameObject`);
     the GameObjectLifecycle tests pass.
  2. `driver.ps1 launch -Harness -Scene Sponza.scene -SettleSeconds 20`, then `close`: `closed code=0`, no assert
     dialog, and the log ends "encountered 0 live resources". Linux released 109 resources at shutdown.
  3. The same with `RenderTest.scene` (GifChat, cameras): 94 released, 59 of them GIF frames, 0 live.
  4. File > New Scene with DrawLineTest open, then `close`: no assert, no D3D11 debug-layer live-object report in the
     debugger output.
  5. The Game (`launch -App Game -Scene RenderTest.scene`, then `close`): `closed code=0`, 0 live resources.

- **A9. Asserts run in Development (the commit after c77934dd; Development was Release then).** `RZE_ASSERTS_ENABLED`
  (new `Utils/DebugUtils/DebugConfig.h`) is on unless `RZE_RETAIL`, so the assert macros, the test hook,
  `LiveObjectPtr`'s dead-handle check, the duplicate-child checks and the Editor's "outside a command" check now run in
  Development. The release CRT has no `_CrtDbgReport`, so `DebugWindows.cpp`'s `ShowAssertReport` has a non-Debug
  branch: the test hook if one is set, otherwise a `MessageBoxA` with Abort/Retry/Ignore (Abort `std::abort()`s, Retry
  breaks, Ignore carries on). None of the Windows code was compiled on Linux; on Linux all three configs built and
  passed 433/433, and Sponza and RenderPerfTest opened and closed in a Development harness with no assert.
  1. A1 must build Development with no errors or warnings from `DebugWindows.cpp`, and from Windows-only code (DX11,
     `Platform/Windows`) whose asserts now compile in Development for the first time: a name an assert uses that only
     exists under `_DEBUG` fails here. Retail must still build (asserts compile out there).
  2. A2 in Development as usual. Then, temporarily, append to `UnitTests/Src/Utils/ByteStreamTests.cpp`:
     `TEST_CASE("TempAssertProbe") { AssertMsg(1 + 1 == 3, "probe"); CHECK(true); }`, build, and run
     `UnitTests.exe -tc=TempAssertProbe` in Debug, Development and Retail. Expect Debug and Development to fail it with
     `Assert: Assertion failed: 1 + 1 == 3` (no dialog), and Retail to pass it. Then
     `git checkout -- RZE/UnitTests/Src/Utils/ByteStreamTests.cpp` (`git status --short` first) and rebuild.
  3. `driver.ps1 launch -Harness -Config development -Scene Sponza.scene -SettleSeconds 20`, then `close`, and the same
     with `RenderPerfTest.scene`: `closed code=0`, and no `Assertion failed` in the newest `RZE\Logs\RZELOG_*.txt`.
     List any that come: they're the first asserts Development has ever run.
  4. **(person)** The Development message box: temporarily add `AssertMsg(false, "probe");` at the top of
     `EditorApp::Initialize`, build Development, run `RZE\_build\development\Editor.exe` under the Visual Studio
     debugger: expect a box titled "RZE: Assertion Failed" with the expression, file and line; Retry stops in the
     debugger on that line, and Ignore carries on to the Editor. Then run it without a debugger and press Abort: the
     process ends. Revert the line and rebuild.

- **A11. Release is now called Development (the commit after 139bfcb1).** `CommonTarget.ConfigurationName` names
  Sharpmake's `Optimization.Release` "Development"; project and solution configurations both take their name from
  the target (`[target.Name]`). Nothing about the build changed, only its name. On Linux the Makefiles'
  `config=development` builds into `_build/development`, and `make help` lists debug, development and retail. The
  `.sln` isn't generated on Linux, so:
  1. After `GenerateSolutions.bat`, `RZE.sln`'s `SolutionConfigurationPlatforms` lists `Debug|x64`, `Development|x64`
     and `Retail|x64`, with no `Release`, and each project's `.vcxproj` has a `Development|x64` configuration that the
     solution's Development maps to (Configuration Manager in Visual Studio shows Development for every project).
  2. `msbuild RZE\RZE.sln /p:Configuration=Development /p:Platform=x64` builds into `RZE\_build\development\`, and
     `driver.ps1 launch -Harness -Config development -Scene DrawLineTest.scene`, then `close`, gives `closed code=0`.
  3. An old `RZE\_build\release\` (and `_project\obj\...\Release` folders) are stale: delete them after the build.
  4. CI's Windows job now has a `development` leg in place of `release`; note whether it passed on the pushed commit.
- **A12. Development is the default (the commit after 7439d711).** `driver.ps1 -Config` defaults to `development`
  and now accepts `retail`, and the skill's MSBuild and test commands use Development. The Editor's Build Game and
  Launch Game build and start Development: `BuildGame.bat` builds `Configuration=Development`, and `GameBuilder.cpp`
  starts `_build\development\Game.exe`. On Linux, `make` with no `config=`, `BuildGame.sh` and Launch Game do the same.
  1. `driver.ps1 launch -Harness -Scene DrawLineTest.scene` with no `-Config` runs `RZE\_build\development\Editor.exe`
     (or EditorHarness; check the process's path), then `close`: `closed code=0`.
  2. `driver.ps1 launch -Harness -Config retail -Scene DrawLineTest.scene`, then `close`: `closed code=0`. The driver
     has never run Retail on Windows before, so report what fails if it doesn't.
  3. `RZE\BuildGame.bat` from a plain PowerShell (no developer prompt): exit code 0, and
     `RZE\_build\development\Game.exe` has the build's time. Then, in the Editor with DrawLineTest open, Launch Game:
     the Game opens in DrawLineTest. Close it with Escape.
  4. **(person)** In Visual Studio, pick Development in the configuration list, close the solution and open it again:
     Development is still selected.

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
- **C6. Close (person).** `driver.ps1 close` passed (2026-11-05). Left: the window's close button and Alt+F4 both
  close it, and with unsaved changes both ask first.
- **C7. Icons (person).** The taskbar passed (2026-11-05). Left: the Alt+Tab icon is RZE's, and the icons are sharp
  at both sizes. A Game built with an icon set in Project Settings shows that icon. EditorHarness's generic icon is
  accepted (it doesn't embed `Editor.rc`).
- **C8. DPI and refresh.**
  - On a monitor scaled above 100 %, the Editor is the same size as before the port (it's DPI-unaware).
  - The frame rate holds at the monitor's rate. Q28 made refresh rates exact, e.g. 59.94, not 59.
- **C9. Monitors.** With `LaunchMonitor` set in `Engine.ini`, the Editor opens on that monitor at its work area's
  top-left. `-console` puts the console beside it.
- **C10. Input states (the commit after e0ac50fe).** `InputHandler` now counts Pressed/Hold in frames rather than
  events, gives bindings each event's own state (an OS key repeat is Hold), and keeps a key or button released in
  the frame it was pressed in down until the next frame, for keys and now mouse buttons too. That deferral moved
  from `Window` into `InputHandler::BeginFrame`, so it also covers `WindowNativeWindows.cpp`'s title-bar path, which
  calls `OnMouseDown`/`OnMouseUp` directly. Passed on Linux: unit tests, and in the harness the gizmo shortcuts
  (W/E/R), Escape clearing the selection, and typing into a field.
  1. `driver.ps1 launch -Harness -Scene DrawLineTest.scene`: select NeckMech, press E, R, W; expect the toolbar's
     Rotate, Scale, Translate to be picked in turn. Esc clears the selection.
  2. **(person)** Click the title bar's maximize button as fast as you can, several times: each click restores or
     maximizes. Then fly the camera (E5) and press E while the right button is held: the camera moves up and the
     gizmo stays as it was.
  3. **(person)** With NeckMech selected and the pointer over the scene view, hold Q for two seconds, past the repeat
     delay: the toolbar's Local/World flips once, not once per repeat.

- **C11. The Game's camera keeps the window's shape (person; the commit after 0bbaedc3).** Cameras no longer store
  an aspect ratio: they frame the main view's `ViewportSize`, which `GameApp` now also sets at startup (Linux gets two
  resize events at startup anyway; check Windows doesn't depend on them). Run `RZE\_build\debug\Game.exe`, then drag
  a window edge to make it much wider, then much taller: Nyra keeps her proportions each time, never stretched. (F1-F3
  resize it too, once C12's fix is in.)
- **C12. The Game takes keyboard and mouse separately (person; the commit after bc8ba734).** The app's input answer
  is now per device (`InputGate`). The Debug Game used to drop all input whenever ImGui was on, so its bindings and
  `PlayerControllerComponent` never ran; now each device goes to the game unless ImGui wants it. The Editor's gate is
  unchanged (both devices while the Scene view is hovered). Passed on Linux: unit tests in all three configs, and in
  the harness Escape clears the selection only over the Scene view, R picks Scale, the wheel zooms.
  1. Run `RZE\_build\debug\Game.exe`. F1 / F2 / F3 resize the window to 1280x720 / 1600x900 / 1920x1080.
  2. Hold W, A, S, D: the camera flies. (The mouse only steers which way WASD moves: `PlayerControllerComponent`
     never turns the view, a known follow-up.)
  3. Put the pointer over ImGui's demo window and press F1: the window resizes (the keyboard still reaches the game).
  4. Escape quits; exit code 0.
  5. Repeat 1, 2 and 4 with `RZE\_build\development\Game.exe`, which has no ImGui: everything goes to the game.
- **C13. Mouse movement is a per-frame delta from InputHandler (person; the commit after C12's).**
  `MouseState::GetPositionDelta()` replaces the two cameras' own previous positions: 0 in the frame the window first
  reports a position, and while the app doesn't take the mouse. Passed on Linux: unit tests in all three configs, and on
  the desktop a right-drag turns the editor camera and a middle-drag orbits, with no jump.
  1. Editor, `DrawLineTest.scene`: right-drag in the scene view turns the camera smoothly; middle-drag orbits;
     Shift+middle-drag pans; Ctrl+middle-drag dollies. No jump on the first frame of any drag.
  2. Start a right-drag, move the pointer out over another panel and back while holding: no jump when it comes back.
  3. Game (Debug): right after launch, hold W without touching the mouse: the camera moves straight ahead (before,
     its direction depended on where the pointer was when it first moved).

### D. Rendering on DX11

- **D5. The D3D11 debug layer's reports reach the log.** Debug builds now read `ID3D11InfoQueue` after every
  present (`DX11Device::ReportDebugMessages`): corruption and errors log as `[Error] D3D11 (<id>): ...`, warnings as
  `[Warning] D3D11 (<id>): ...`. Nothing read them before, so these are the first look. CI's smoke test opens `Sponza.scene`
  on WARP and only watches it load (its Windows Debug job lists any it sees); this check covers a real GPU and what
  the Editor does after that. In a **Debug** Editor (`driver.ps1 launch -Harness`), open `Sponza.scene` and
  `RenderPerfTest.scene` (probes and shadows), let each settle 20 s, select an object, open the Content Browser so
  thumbnails render, then `close`. From the newest
  `RZE\Logs\RZELOG_*.txt`, report every distinct `D3D11 (` line with how often it came (e.g. `Select-String 'D3D11 \('
  | Group-Object Line | Sort Count -Desc`), and say which scene or action brought it. Expect no line saying the info
  queue isn't available. Also check a Development Editor logs no `D3D11 (` line at all.
- **D6. Vertex and pixel shader creation.** `DX11VertexShader::Create` no longer creates an input layout for a
  shader without inputs, and `Release()` frees the input layout (CI confirmed the `D3D11 (420)` warning is gone,
  d8d276ac). VS and PS `Create` now release their compile blobs, and a failed compile logs and returns instead of
  dereferencing a null blob. On a real GPU:
  - Sponza, DrawLineTest, SpaceHangar and RenderTest render as they did before these commits: textured, shadowed,
    with GTAO and FXAA. The full-screen passes (FXAA, GTAO, the final copy) are the shaders without inputs, so a
    broken one shows as a black or unprocessed scene view.
  - **Compile failure:** add a syntax error to a pixel shader's HLSL (e.g. `Assets/Shaders/Pixel_FXAA.hlsl`), launch a Debug
    Editor on Sponza: expect `[Error] Couldn't compile shader [...]` in the log and no crash in `Create` (an assert
    or D3D error later, from drawing with no shader, is expected). Revert the HLSL and `git diff` it clean.
  - At a Debug Editor's `close`, the log has no new `D3D11 (` lines.
- **D7. The render command arena (MemArena).** Since 10caafa0 (and 89ab685a, 6e5ea004 and the commit after it),
  `Rendering::MemArena` owns its buffers with `unique_ptr<Byte[]>`, spills to the heap when it's full instead of
  writing past the end, and aligns each allocation (`alignof(T)` for commands, `max_align_t` for raw data, which is 8
  on MSVC and 16 on Linux). `MemoryUtils::AlignSize` takes `size_t` now, so `DX11ConstantBuffer::Allocate` casts its
  `ByteWidth` to `UINT`.
  - **Build:** A1 covers `DX11ConstantBuffer.cpp`; expect no C4267 or other warning from it.
  - **Tests:** `UnitTests.exe -sf="*MemArenaTests*,*MemoryUtilsTests*"` in Debug and Development: expect 10 test cases,
    all passing.
    The alignment cases use `alignof(std::max_align_t)`, so they test 8-byte alignment here.
  - **A full arena on DX11:** in `RZE\Modules\Rendering\Src\Rendering\Renderer.cpp`, change `MEM_ARENA_SIZE` to
    `MemoryUtils::Megabytes(1)`, build Debug, `driver.ps1 launch -Harness -Scene Sponza.scene -SettleSeconds 20`,
    `shot`, `close`. Expect: Sponza renders, `closed code=0`, no assert, and `[Warning] Rendering memory arena was
    full: ...` lines in the newest `RZE\Logs\RZELOG_*.txt` (Linux had 47). Some textures fail with `exceeds the
    arena`; that's expected at 1 MB, which is under `AsyncOperationManager`'s 8 MB safety margin. Then
    `git checkout -- RZE/Modules/Rendering/Src/Rendering/Renderer.cpp` (that one file; `git status --short` first),
    and rebuild.
  - **At the real size:** a Debug Editor on Sponza, `close`: `closed code=0` and no `arena was full` line.
  - **Memory (since the commit after 7588d9e4):** the arena's two 2048 MB buffers are no longer zeroed at startup, so
    only the pages frames use become resident. With a Debug Editor on Sponza (`launch -Harness`, settled 20 s), report
    `Get-Process EditorHarness | Select WorkingSet64, PrivateMemorySize64` in MB, and the same at the commit before
    for comparison if it's quick. Expect the working set to be about 4 GB lower than before (Linux: 4.4 GB to 0.3 GB
    resident). Private bytes (commit charge) are expected to stay about 4 GB higher than the working set, since
    Windows charges the whole allocation up front: say if it doesn't.
- **D8. Anti-aliasing moved to its own stage (the commit after db92fb19).** FXAA left `PostProcessRenderStage` for
  `AntiAliasingStage`, with its own cbuffer (`FXAAParams` in `Pixel_FXAA.hlsl`, `FXAAParamsLayout`), and
  `PostProcessParams` lost FXAA's fields. The AO debug view's overlay moved to `DebugViewStage`, and the full-screen
  passes bind their inputs through `DrawFullScreenPass`. The image must not change. On Linux, Sponza at AA Off, Low,
  Medium and High and the AO debug view matched the commit before to within the run-to-run noise (at most 215 px of
  the scene view, at most 4 levels; two runs of one build differ by up to 202 px).
  - The Debug Editor's log shows `Render pipeline: ... PostProcessRenderStage -> AntiAliasingStage -> DebugViewStage
    -> PickRenderStage -> DebugDrawRenderStage -> ...`.
  - On Sponza (`launch -Harness -Scene Sponza.scene -SettleSeconds 20`), open Edit > Settings > Scene View and click
    through Anti-aliasing Off, Low, Medium and High, then Show: Ambient occlusion and back to Lit, with a `shot` of each
    (close the Settings window first). Expect FXAA to smooth edges at every level but Off, and the AO view to be grey
    occlusion over the whole scene view, with the grid and icons on top. If a quick worktree build of db92fb19 is
    possible, compare the same shots; otherwise judge them by eye.
  - The game (File > Launch Game...) shows the scene as the editor does: `FinalRenderTargetStage` now draws through
    `DrawFullScreenPass`.
  - At a Debug Editor's `close`, no new `D3D11 (` lines (D5), in particular none about a slot or a constant buffer.
- **D9. RG8 textures, texels in any format, and the s3 sampler (the commit after ed7fea98).** None of this was compiled
  on Linux: `DX11Device.cpp`, `DX11Device.h`, `DX11.h` and `DX11TextureBuffer2D.cpp` changed. `ETextureFormat` gained
  `RG8_UNORM` (`DXGI_FORMAT_R8G8_UNORM`). A texture made with texels takes its row pitch from
  `GetBytesPerTexel(DXGI_FORMAT)`, which moved out of `DX11Device.cpp`'s anonymous namespace, is declared in `DX11.h`,
  and now knows `R8G8_UNORM` (2). The RGBA8-only assert there is gone. `DX11Device` makes `mPointClampSamplerState`
  (point, clamped) and binds it at s3 for PS and CS in `BeginDrawSet`, beside s1 and s2. Nothing samples s3 yet (SMAA
  will). On Linux, Sponza matched ed7fea98 within the noise, and a temporary round trip read back the exact bytes.
  - It builds (section A). `Shutdown` releases the new sampler; if the debug layer lists live objects at `close`, it
    isn't among them.
  - Sponza (`launch -Harness -Scene Sponza.scene -SettleSeconds 20`) looks as before: every texture loaded from disk is
    RGBA8 and goes through the new pitch (`Width * 4`, as before).
  - The round trip: temporarily, at the top of `AntiAliasingStage::Render`, make three shader-resource textures with
    `Renderer::CreateTextureBuffer2D(TexelBuffer(malloc'd bytes, &std::free), params)` (MipLevels 1, SampleCount 1):
    R8_UNORM 5x3 holding bytes 1..15, RG8_UNORM 3x2 holding 101..112, RGBA8_UNORM 3x2 holding 151..174. Two calls
    later, `Renderer::ReadTexture` each, and once `IsReady`, log whether `Data` is exactly those bytes. Expect all three
    to match; a wrong pitch shows as a row starting at the wrong byte. Remove the code afterwards.
  - At a Debug Editor's `close`, no new `D3D11 (` lines (D5).
- **D10. SMAA (the commit after 3c54e95a).** A second anti-aliasing method in `AntiAliasingStage`: edge detection,
  blending weights and neighbourhood blending, through `Common/SMAA/SMAA.hlsli` (our setup: `SMAA_CUSTOM_SL`, samplers
  at s2 and s3, the `SMAAParams` cbuffer) around upstream's `SMAA.hlsl`. **fxc has never compiled these:** the spike
  and Linux only ran glslang and the GL driver. Shaders compile at start-up, so a failure shows when the Editor
  starts, not in the build.
  - At start-up, the Debug Editor's log has no shader compile errors for `Pixel_SMAAEdgeDetection`,
    `Pixel_SMAABlendingWeights`, `Pixel_SMAABlendingWeightsNoDiagonals` or `Pixel_SMAANeighborhoodBlending`.
    `SMAA.hlsl` is CRLF with one Latin-1 byte (`0xB7`) in a comment, kept byte for byte; if fxc warns about that byte,
    note it rather than editing the file. `SMAA.hlsli` includes `../FullScreenQuad.hlsli`, the first `../` include in
    the shaders.
  - On Sponza (`launch -Harness -Scene Sponza.scene -SettleSeconds 20`), Edit > Settings > Scene View: Anti-aliasing
    now offers Off, FXAA and SMAA, and Quality offers Ultra only with SMAA (with FXAA and Ultra set, it shows High).
    `shot` SMAA Low, Medium, High and Ultra and FXAA High. Expect SMAA to smooth stair-steps (column edges, the arch
    blocks) while texture detail stays as sharp as with Off; FXAA visibly softens the floor's texture. On Linux, SMAA
    High changed about 34k pixels of the scene view against Off, and FXAA High about 203k.
  - Narrow the scene view by dragging the splitter to the Scene panel left (e.g. 550 px), with SMAA Ultra. Expect no
    line or seam along the scene view's new right edge or its bottom edge.
  - Select the Camera: its preview in the Object Inspector renders correctly with SMAA (it's SMAA High, the default
    for other views since the commit after 79165770).
  - At a Debug Editor's `close`, no new `D3D11 (` lines (D5), in particular none about render target and shader
    resource bindings (the edges and weights textures are each written by one pass and read by the next).
- **D11. Resizing the window frees the old render target (the commit after 3c2a07d8).** A TODO in
  `EditorApp::CreateRenderTarget` suspected a leak and was removed: the target's handles hold `shared_ptr`s to the
  driver textures, and `~DX11TextureBuffer2D` calls `Release()`. Each window resize makes a new target (about 57 MB at
  2560x1392: RGBA8 colour, RGBA16F scene colour, depth). Nothing reports live D3D objects, so measure GPU memory:
  1. `driver.ps1 launch -Scene DrawLineTest.scene` (not `-Harness`: this needs the real title bar), let it settle.
  2. Read `(Get-Counter "\GPU Process Memory(pid_$pid*)\Dedicated Usage").CounterSamples | Measure CookedValue -Sum`
     for the Editor's PID (`driver.ps1 alive`).
  3. Click the title bar's maximize/restore button 20 times (each is a resize), wait 2 s, read the counter again.
  Expect the two readings within about 60 MB of each other (one target's worth of slack); a leak shows as about
  1 GB more. Then `close`: `closed code=0`.

### E. Editor features

- **E3. Title during the splash.** The title after loading and the Game's title passed (2026-11-05). Left: while the
  splash shows, the Editor's taskbar entry already says `RZEStudio`.
- **E4. The Recycle Bin restores a deleted asset to its own folder.** Since the commit after 604189cd, an asset
  deleted in the Content Browser goes to the Recycle Bin from where it was, not from `ProjectData\.Trash`: each trash
  entry records the path in an `Original` file, and `AssetFiles::Recycle` / `EmptyTrash` move the item back there just
  before `SHFileOperationW`. Passed on Linux, where the Trash is now written by the Editor itself (the freedesktop spec, not `gio`).
  1. Make `RZE\Assets\RecycleProbe.txt` (any text). `driver.ps1 launch -Harness -Scene DrawLineTest.scene`, open the
     Content Browser, type `RecycleProbe` in its search box, select the file, press Delete and click **Delete**.
  2. Expect `RZE\ProjectData\.Trash\1\Original` to hold `Assets/RecycleProbe.txt`, and the file to be gone from
     `Assets`. Then `close`: expect `closed code=0` and no `RZE\ProjectData\.Trash`.
  3. Find it in the Recycle Bin: `$item = (New-Object -ComObject Shell.Application).NameSpace(10).Items() | ? Name -like 'RecycleProbe*'`.
     Expect `$item.ExtendedProperty('System.Recycle.DeletedFrom')` to be `...\RZE\Assets`, not `...\ProjectData\.Trash\1\Item`.
  4. Restore it (`$item.InvokeVerb('undelete')`; if that verb isn't found, Restore in Explorer, **person**). Expect
     `RZE\Assets\RecycleProbe.txt` back with its text. Then `Remove-Item` it, and check `git status` shows nothing new
     under `Assets`.
- **E5. Flying the editor camera (person; e0ac50fe).** `EditorCameraComponent` now asks whether each
  key and mouse button is down (`IsDownThisFrame`) instead of comparing `GetButtonState` with Pressed (and, for W,
  Hold). The harness can't hold a button while pressing keys, so this needs hands. With the right mouse button held
  in the scene view, hold each of W, A, S, D, Q and E for over a second, past the keyboard's repeat delay: expect
  smooth movement the whole time, with no stop or stutter when key repeat starts. Holding R speeds it up and Space
  slows it down for as long as they're held. Right-drag looks around, middle-drag orbits (Shift pans, Ctrl dollies),
  and either one stops a double-click focus animation part way.

### F. Profiling

- **F1. Optick capture (person).** The Editor's port (31318) accepting a connection passed (2026-11-05). Left: with
  a Debug or Development Editor running, `Tools\Optick\Optick.exe` connects and takes a capture with frames in it.
- **F2. What `/Ob1` costs Windows Development (measurement; the commit after 103ab018, when it was called Release).**
  `BaseProject.ConfigureRelease` sets `Inline.OnlyInline` (`/Ob1`: only functions marked `inline`), over Sharpmake's
  Release default `AnySuitable` (`/Ob2`). On Linux, the clang counterpart (`-finline-hint-functions`) cost Linux Release
  (now Development) **+38% update time** (0.582 to 0.804 ms mean) on `RenderPerfTest.scene`, with frame time unchanged
  (GPU-bound at about 2.0 ms). This measures whether Windows pays the same, to decide between keeping `/Ob1` (easier
  stepping in Development) and `/Ob2`. Nothing is committed.
  1. Build Development as it is, then copy `RZE\_build\development\Editor.exe` into a new `RZE\_build\perf-Ob1\` (the
     exe finds the source tree from the `_build\` in its path, so it runs from there).
  2. Temporarily change `Inline.OnlyInline` to `Inline.AnySuitable` in `ConfigureRelease`
     (`RZE/Make/Sharpmake/BaseProject.sharpmake.cs`), run `GenerateSolutions.bat`, **Rebuild** Development
     (`/t:Rebuild`), copy the exe into `RZE\_build\perf-Ob2\`. Then `git checkout --
     RZE/Make/Sharpmake/BaseProject.sharpmake.cs` (`git status --short` first), run `GenerateSolutions.bat` again and
     Rebuild Development, so `_build\development` is back to `/Ob1`.
  3. Back up `RZE\Config\imgui.ini`. Run Ob1, Ob2, Ob1, Ob2, Ob1, Ob2 on the same monitor, copying the backup over
     `imgui.ini` before each run, with the working directory at the exe's folder:
     `.\Editor.exe -scene RenderPerfTest.scene -perfcapture <out>.csv -perfwarmup 20 -perfduration 30`. The capture
     turns vsync off and exits by itself. Each line of the CSV is `frameMs,updateMs`. Restore `imgui.ini` at the end
     and delete the two `perf-*` folders.
  4. Report frame time and update time for each side: mean, median, p95 and p99 (averaged over its 3 runs), the spread
     (the range of the run means as a % of their mean), and the % change from Ob1 to Ob2. Say plainly when a difference
     is within the spread. Optionally **(person)**: in Visual Studio, step through `LightComponent::Update`
     (`LightComponent.cpp`, 16 lights in the scene) in each build and say whether `/Ob2` makes stepping noticeably
     worse.

## Results

Results already read by Linux Claude are removed from this table; they're in this file's git history (the last full
table is at ce15cced).

| ID | Date | Commit | Result | Notes |
|---|---|---|---|---|
