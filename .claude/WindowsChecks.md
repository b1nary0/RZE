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

- **A1. Generate and build.** Run `GenerateSolutions.bat`, then build Debug and Release (`/W3 /WX`). Expect no errors
  or warnings. **Since the commit after 3c2a07d8:** every component class is `final`, and `GameObjectComponent<T>`'s
  constructor has `static_assert(std::is_final_v<T>)`; expect MSVC to accept both, with no new warnings.
- **A2. Unit tests.** `RZE\_build\debug\UnitTests.exe` and `RZE\_build\release\UnitTests.exe`. Expect exit code 0.
  **Since the commit after f5faea2e:** the tests that write into the editor's trash and thumbnail folders remove the
  empty folders they made (`RemoveEmptyFolders`, new `UnitTests/Src/TestFolder.cpp`), so after both runs
  `RZE\_build\debug\ProjectData` and `RZE\_build\release\ProjectData` don't exist (A10 found an empty one). A tree
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
  queue isn't available. Also check a Release Editor logs no `D3D11 (` line at all.
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
  - **Tests:** `UnitTests.exe -sf="*MemArenaTests*,*MemoryUtilsTests*"` in Debug and Release: expect 10 test cases,
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

### F. Profiling

- **F1. Optick capture (person).** The Editor's port (31318) accepting a connection passed (2026-11-05). Left: with
  a Debug or Release Editor running, `Tools\Optick\Optick.exe` connects and takes a capture with frames in it.

## Results

Results already read by Linux Claude are removed from this table; they're in this file's git history (the last full
table is at ce15cced).

| ID | Date | Commit | Result | Notes |
|---|---|---|---|---|
