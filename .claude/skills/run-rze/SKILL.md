---
name: run-rze
description: Build, run, and drive the RZE engine's Editor (RZEStudio) on Windows or Linux. Use when asked to start or launch the editor, build the solution, burn assets, take a screenshot of the editor or scene view, click around the ImGui UI, or verify a rendering/editor change in the running app.
---

RZE is a C++ engine with a Dear ImGui editor: DX11 on Windows, OpenGL on Linux. An agent drives the running Editor
with real mouse and keyboard input and client-area PNG screenshots. Each call does one action; the editor process holds
the state between calls.

- **Windows:** `.claude/skills/run-rze/driver.ps1` (user32 input). The sections below, up to **Linux**, are Windows'.
  Paths are relative to the repo root (`C:\dev\RZE`); the commands are PowerShell 5.1.
- **Linux (KDE Plasma, Wayland):** `.claude/skills/run-rze/driver.py`, with the same commands. See **Linux** at the end.

## Prerequisites

- Visual Studio with the C++ workload. This was verified with VS 18 Community, MSBuild at `C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe`.
- An interactive desktop session. The driver moves the real cursor and steals focus, so don't use the machine while it runs.

## Build

From the repo root, in this order:

```powershell
Push-Location RZE; & .\GenerateSolutions.bat; Pop-Location
$msbuild = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -find MSBuild\**\Bin\MSBuild.exe
& $msbuild RZE\RZE.sln /p:Configuration=Debug /p:Platform=x64 /m /v:minimal /nologo
& .\RZE\_build\debug\SourceAssetBurner.exe
```

| step | why |
|---|---|
| `GenerateSolutions.bat` | Sharpmake generates `.sln`/`.vcxproj`, which are gitignored. **Re-run it after adding or removing any source file**, or the new file won't compile. |
| MSBuild | Builds Debug x64 into `RZE\_build\debug\`. A clean incremental build is fast. |
| `SourceAssetBurner.exe` | Imports `RZE\Assets\3D\**` into `RZE\ProjectData\` (gitignored). Needed once after cloning. Takes about 15 s and writes into `RZE\ProjectData` wherever you run it from. |
## Run (agent path)

```powershell
$d = '.\.claude\skills\run-rze\driver.ps1'
& $d launch -Scene DrawLineTest.scene      # waits for the window title, then 5 s for assets to stream in
& $d shot                                  # -> $env:TEMP\rze-shots\<timestamp>.png
& $d click -X 60 -Y 95                     # select "NeckMech" in the Scene panel
& $d click -X 2328 -Y 263                  # tick "Mesh Bounds" (Debug Draw) in Object Inspector
& $d click -X 2372 -Y 143; & $d key -Keys '^a45{ENTER}'   # set Rotation Y = 45
& $d shot -Out "$env:TEMP\rze-shots\flow.png"
& $d alive                                 # running pid=... responding=True
& $d quit                                  # kills Editor.exe; the edits above are discarded
```

`quit` kills the process, so the engine's shutdown never runs. To check that too, end a run with `close`, which closes
the window the way a user would:

```powershell
& $d launch -Scene Sponza.scene -SettleSeconds 20
& $d close                                 # closed code=0, or fails with the crash's exit code (e.g. 0xC0000005)
```

**Always Read the screenshot afterwards and look at it.** Coordinates are client-area physical pixels, the same space as the screenshot.

| command | what it does |
|---|---|
| `launch [-Scene X.scene] [-Config debug\|release] [-SettleSeconds 5]` | Starts `Editor.exe -scene X` with the working directory set to `RZE\_build\<config>`. Refuses to start if an editor is already running. |
| `shot [-Out path.png]` | Captures the client area only. Defaults to `$env:TEMP\rze-shots\`. |
| `click -X -Y` | Left click. |
| `dblclick -X -Y` | Two left clicks close enough together to count as a double-click (two `click` calls are too far apart). Double-clicking an object in the Scene panel focuses the camera on it. |
| `drag -X -Y -X2 -Y2` | Left-button drag in 10 steps. Verified on the translate gizmo's X arrow. |
| `key -Keys '...'` | `SendKeys` syntax. `'^a<value>{ENTER}'` replaces an ImGui number field. |
| `info` | Window title and client size. |
| `alive` | `running pid=N responding=True/False` or `not running`. |
| `close [-TimeoutSeconds 30]` | Sends the window `WM_CLOSE`, like its close button, and waits for the process to exit. Prints `closed code=0`. Fails if the editor exits with any other code (a crash while shutting down), or is still open after the timeout. In that case it's left running, so `shot` can show why (the unsaved-changes prompt, a Debug assert dialog), then `quit`. |
| `quit` | Kills the editor and polls until the process is gone. |

Scenes live in `RZE\Assets\Scenes\`:
- `DrawLineTest.scene`: small; NeckMech (5 sub-meshes), M4, and a directional light.
- `Sponza.scene`: heavy; Sponza has 25 sub-meshes. Use `-SettleSeconds 20`.

### Layout reference (maximized, 2560x1392 client)

The editor starts maximized on a 2560-wide monitor. ImGui panels scale with the window, so **re-derive coordinates from a fresh `shot` on any other monitor.**

The editor has no Windows title bar: its menu bar is the title bar (40 px; the OS caption is 31), with the logo, the
menus, the window's title and its own minimize, maximize and close buttons. The client area starts at the top of the
screen, and `shot` includes the bar. Rows marked "(+16)" were moved down by the taller bar (it replaced a 24 px menu bar) without being clicked again.

✓ = clicked through the driver and confirmed. Unmarked = measured from a screenshot only.

| target | client coords |
|---|---|
| Title bar: minimize / maximize (restore) / close buttons | (2445 / 2491 / 2537, 20) (+5). Close asks about unsaved changes, like `close` |
| Title bar: empty part (moves the window; double-click maximizes) | e.g. (1500, 20) (+5) |
| Scene panel rows (Camera, DirectionalLight, ...) | x≈38–60, y = 75 + 18·row (row 0 = Camera). M4 = row 3 → (38, 129); NeckMech = row 2 → (60, 111); Sponza in Sponza.scene → (51, 111) (+16) |
| Object Inspector: Position X / Y / Z field | (2310 / 2372 / 2435, 118) (+16) |
| Object Inspector: Rotation X / Y / Z field | (2310 / 2372 / 2435, 159) (+16) |
| Object Inspector: Scale X / Y / Z field | (2310 / 2372 / 2435, 202) (+16) |
| RenderComponent "Mesh Bounds" checkbox (Debug Draw section) | (2328, 279) (+16) |
| RenderComponent "Sub-Mesh Bounds" checkbox (Debug Draw section) | (2328, 303) (+16) |
| DrawLineTest: NeckMech translate gizmo, X arrow (after selecting NeckMech) | measured before the title bar change as drag (1555, 1038) → (1755, 1038); the scene view has since grown, so measure again from a `shot` |

The status bar at the bottom shows `RenderCommand MemArena: <N> KB`, which is the render command memory used per frame. Compare it before and after a change to check render-path cost: for example, 600 debug-line vertices add about 14 KB.

## Run (human path)

Open `RZE\RZE.sln` in Visual Studio, set **Editor** as the startup project, and press F5. `RZE\Editor\_project\Editor.args.json` has preset `-scene` arguments.

## Test

The solution builds `UnitTests.exe` (doctest). Run it after any Editor or Engine change:

```powershell
& .\RZE\_build\debug\UnitTests.exe    # exit code 0 = all passed; -tc="Undo*" runs matching test cases
```

It covers the undo history, parking, copy snapshots, selection, interaction tracking and the property registry, with no running engine. CI (`.github/workflows/Windows.yml`) runs it in Debug and Release after the build. Anything that needs a window or the renderer (selection restore, the main camera, rendering) still means building, then driving the editor as above.

## Gotchas

- **Never `ShowWindow(SW_RESTORE)` the editor to focus it.** On a maximized window it un-maximizes it (2560x1392 → 1584x892), the whole ImGui layout reflows, and every coordinate you measured is wrong. The driver only restores when the window is minimized.
- **Debug draws are stripped from Retail builds.** `DebugDrawRenderStage` (debug lines, bounds boxes, the light's direction line, camera frustums) is only added when `RZE_RETAIL` isn't defined, so Debug and Release both show them.
- **Everything reads assets from the source tree, not from `_build\<config>`.** The Editor, Game and SourceAssetBurner set `EDirectoryContext::Source`, so `Filepath` resolves paths against the folder above `_build\` (e.g. `RZE\Assets\Shaders\...`), whatever the working directory. Shader and asset edits take effect on the next launch with no copy step. This also means the shader in `RZE\Assets` is what runs: to capture a "before" image, revert or disable the change in the source file itself. Any `Assets\`/`Config\`/`ProjectData\` folders left under `_build\<config>` are stale copies from the old `AssetCpy.bat` and aren't read. `EDirectoryContext::Runtime` (beside the exe) is reserved for a packaged game.
- **A `Filepath` built during static init resolves before `main` sets the context**, so it points beside the exe. Build paths where they're used, or as function-local statics.
- **Large screenshots are downscaled when Read.** A 2560-wide PNG shows at 2000 px with a note to "multiply coordinates by 1.28". Measure positions in the displayed image, then multiply by that factor before passing them to `click`.
- **The window title appears before the scene finishes loading.** Meshes stream in afterwards. `-SettleSeconds` covers this: 5 is enough for DrawLineTest, Sponza needs about 20.
- **`quit` skips shutdown entirely, so it can't catch a crash on exit.** End at least one run per change with `close`. After editing the scene, `close` stops at the unsaved-changes prompt instead; use `quit` there, or answer the prompt.
- **`Stop-Process` returns about 2 s before `Editor.exe` actually exits.** The render thread and D3D device are still tearing down. `quit` polls for this; don't `launch` immediately after a bare `Stop-Process`.
- **The light's line in DrawLineTest is off-screen at load.** Move the DirectionalLight onto NeckMech (Position ≈ -22.7, 14, -9) to see it.
- **The camera starts inside Sponza.** Bounds boxes there look like long lines crossing the view, not boxes. Use DrawLineTest to judge box shapes.

## Troubleshooting

- **`'GenerateSolutions.bat' is not recognized as an internal or external command`**: you ran it through `cmd /c "cd /d ...\RZE & call GenerateSolutions.bat"` from PowerShell. That failed in this environment even though the file exists. Use `Push-Location RZE; & .\GenerateSolutions.bat` instead.
- **Clicks land in the wrong place after the first one**: something un-maximized the window. Check `& $d info` for client size. Either re-maximize it or `quit` and `launch` again.
- **`Editor.exe already running. Run: driver.ps1 quit`**: a previous session left one open. Run `& $d quit`.

## Linux (KDE Plasma on Wayland)

### Build

From the repo root:

```bash
(cd RZE && ./GenerateSolutions.sh)              # again after adding or removing any source file
make -C RZE config=debug -j"$(nproc)"           # every project compiles at once, then links
./RZE/_build/debug/SourceAssetBurner            # once after cloning; about 4 s
./RZE/_build/debug/UnitTests                    # exit code 0 = all passed
```

Setup (once): `sudo dnf install clang lld make dotnet-sdk-8.0 SDL3-devel libshaderc-devel assimp-devel`, as in the README.
Warnings are errors (`-Werror`), as on Windows.

### Drive

**Prefer `launch --harness`** for driving the Editor. It runs `EditorHarness` (`RZE/EditorHarness`, the Editor plus
a harness beside it): input, screenshots and questions go through the app itself, as JSON lines on its stdin.
- Your cursor never moves, and focus never changes; the session has no input devices at all.
- The window can be covered or unfocused.
- `shot` is the frame itself, read back from the back buffer, not a screen capture.
- `state` and `object` answer what the Editor holds, so checks don't depend on reading pixels.
- Real mouse and keyboard input is dropped while it runs (`realinput on` lets it through).

Use OS input (no `--harness`) only for what needs the real desktop: the compositor, window decorations and focus,
Snap Layouts, or RenderDoc's capture key.

```bash
d=".claude/skills/run-rze/driver.py"
$d launch --harness --scene DrawLineTest.scene   # waits until the Editor is maximized, then 5 s for assets
$d click 2317 123                                # select NeckMech
$d click 2444 783; $d key '^a45{ENTER}'          # Rotation Y = 45
$d object NeckMech                               # "rotation": [0, 45, 0], components, parent, GUID
$d state                                         # scene, modified, selection, undo, objects
$d shot                                          # the frame itself, UI and all
$d key '^z'; $d close                            # "closed code=0" once nothing's unsaved
```

The same commands without `--harness` use real input instead:

```bash
$d launch --scene DrawLineTest.scene          # waits for "RZEStudio - ..." in the title, then 5 s for assets
$d shot                                       # -> $TMPDIR/rze-shots/<timestamp>.png (usually /tmp), client area only
$d click 2317 123                             # select NeckMech in the Scene panel
$d click 2444 783; $d key '^a45{ENTER}'       # Rotation Y = 45
$d log --grep error                           # the Editor's output (stdout and stderr)
$d close                                      # closes the window as a user would: "closed code=0"
```

`launch` starts a helper for the session, which holds the virtual input devices and the Editor's process. Every other
command returns in well under a second. Coordinates are **client-area pixels**, the same space as `shot`'s images; Read
downscales a 2560-wide image to 2000, so multiply what you measure by 1.28.

| command | what it does |
|---|---|
| `launch [--harness] [--scene X.scene] [--config debug\|release\|retail] [--app Editor\|Game] [--settle 5] [--keep-imgui-ini] [-- args...]` | Starts the app under the driver. Refuses if one is running. Backs up `RZE/Config/imgui.ini` first (below). `--harness` runs EditorHarness (above); it's the Editor's only, and can't be combined with `--renderdoc`. |
| `state`, `object NAME`, `realinput on\|off` | Harness sessions only: the Editor's state (scene, modified, selection, undo, objects); one object's GUID, parent, components and transform; whether the real mouse and keyboard reach the app. |
| `shot [--out path.png]` | With `--harness`, the frame read back from the back buffer (about 1 s, as it's encoded). Otherwise, it focuses the app and captures its client area (spectacle). |
| `click X Y [--button left\|right\|middle]`, `dblclick X Y`, `move X Y` | A click, a double-click, a hover. |
| `drag X Y X2 Y2 [--button ...]` | A drag in 10 steps. |
| `scroll X Y N` | N wheel notches at (X, Y); + is away from you (zooms the scene view in). |
| `key KEYS` | SendKeys syntax, as `driver.ps1` takes: `^` Ctrl, `+` Shift, `%` Alt, `{ENTER}` `{TAB 3}` `{F5}` `{ESC}`, `~` Enter, `{+}` for a literal +. US layout. |
| `info`, `alive` | The window's title and client area; `running pid=N` or `not running`. |
| `windows [--all]` | Windows opened since `launch` (e.g. the Game, or Dolphin from "Show in Explorer"). |
| `close-window CAPTION` | Closes a window opened since `launch`, by its exact caption. Never one that was already open. |
| `close [--timeout 30]` | Closes the app's window as a user would and waits: `closed code=0`, or an error with the code. Stops at the unsaved-changes prompt (see `shot`), so `quit` after edits. |
| `quit` | Kills the app, like `driver.ps1 quit`. |
| `log [-n 40] [--grep TEXT] [--engine]` | The app's output. A Game launched from the Editor writes here too. `--engine` reads the newest `RZE/Logs/RZELOG_*.txt` instead. |
| `clip get`, `clip set TEXT` | The system clipboard (wl-paste, wl-copy), to check ImGui's copy and paste. |
| `optick [--seconds 2] [--out raw.bin]` | Takes an Optick capture over the network, as Optick's viewer does, and counts what arrived. Debug and Release only. |
| `launch --renderdoc ...`, then `renderdoc-capture [--timeout 30]` | Runs the app under `renderdoccmd` (`sudo dnf install renderdoc`), then captures the next frame: presses RenderDoc's capture key (F12), waits for the `.rdc`, saves its thumbnail beside it and counts its draws, dispatches, debug groups and GL calls. Captures go to `$XDG_RUNTIME_DIR/rze-driver/renderdoc/`; open one with `qrenderdoc <file.rdc>`. |

How it works, and what to know:

- **`imgui.ini` is put back after every session.** Each Editor run rewrites `RZE/Config/imgui.ini`, which often holds
  a layout the user means to keep (CLAUDE.md). `launch` copies it aside; `close` and `quit` restore it once the app has
  exited (their output says `imgui.ini restored`), and so does the helper if the session ends any other way. If a
  session died before it could, the next `launch` restores that backup first and says so. Pass `--keep-imgui-ini`
  only when the run is meant to change the layout.

- **It moves the real pointer and takes focus**, like `driver.ps1`. Don't use the mouse while it's acting. Before any
  input it checks, through KWin, that the app's window is active and the point is inside it, so input can't land in
  another window.
- **Input is real**: virtual uinput devices (an absolute pointer, so clicks land on the exact pixel, and a keyboard)
  that KWin treats like any others. Your user needs write access to `/dev/uinput` (an ACL on this machine).
- **KWin finds, focuses and closes windows** through scripts loaded over D-Bus. It needs a KDE Plasma session; GNOME
  isn't supported.
- **A RenderDoc session runs on XWayland** (`SDL_VIDEO_DRIVER=x11`), with SDL's GL-drawn window surfaces off
  (`SDL_FRAMEBUFFER_ACCELERATION=0`). RenderDoc disables capturing for the whole process ("captures disabled" in its
  overlay) once anything calls GL it doesn't support, and SDL draws the splash screen's window surface with
  fixed-function GL; this RenderDoc build also only reads its capture key through xlib. RenderDoc deletes its log
  (`/tmp/RenderDoc/*.log`) when the app exits cleanly, so read it while the session runs.
- `close` and `quit` end the session, and everything it started: the app runs in a process group of its own, so a
  Game launched from the Editor, or the app under `renderdoccmd`, goes with it. A Game launched with **File > Launch Game...** shows up in `windows` (caption
  `Game Title`, from Engine.ini) within a few seconds; `close-window "Game Title"` ends it.
- **Launch Game with unsaved changes opens the Unsaved Changes prompt**, a modal that ignores clicks elsewhere. The
  panels look dimmed in `shot` while it's open.
- **ImGui's `{ESC}` cancels a field's edit**: end a typed value with `{ENTER}`.
- Delete in the Content Browser moves the file to `ProjectData/.Trash/` (for undo) and on to the system trash
  (`~/.local/share/Trash`) when the Editor closes.
- The layout reference above (Windows) holds on Linux too, for a maximized 2560x1398 client area: the Scene panel's
  rows are at x = 2317, y = 78 / 101 / 123 / 145 (Camera, DirectionalLight, NeckMech, M4 in DrawLineTest); the File
  menu at (90, 19); the Content Browser tab at (58, 1362).
