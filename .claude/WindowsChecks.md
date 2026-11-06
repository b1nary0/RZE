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
  or warnings.
- **A2. Unit tests.** `RZE\_build\debug\UnitTests.exe` and `RZE\_build\release\UnitTests.exe`. Expect exit code 0.
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

### E. Editor features

- **E3. Title during the splash.** The title after loading and the Game's title passed (2026-11-05). Left: while the
  splash shows, the Editor's taskbar entry already says `RZEStudio`.

### F. Profiling

- **F1. Optick capture (person).** The Editor's port (31318) accepting a connection passed (2026-11-05). Left: with
  a Debug or Release Editor running, `Tools\Optick\Optick.exe` connects and takes a capture with frames in it.

## Results

Results already read by Linux Claude are removed from this table; they're in this file's git history (the last full
table is at ce15cced).

| ID | Date | Commit | Result | Notes |
|---|---|---|---|---|
