---
name: run-rze
description: Build, run, and drive the RZE engine's Editor (RZEStudio). Use when asked to start or launch the editor, build the solution, burn/copy assets, take a screenshot of the editor or scene view, click around the ImGui UI, or verify a rendering/editor change in the running app.
---

RZE is a Windows-only C++ / DX11 engine with a Dear ImGui editor. An agent drives the running `Editor.exe` with
`.claude/skills/run-rze/driver.ps1`: real mouse and keyboard input through user32, plus client-area PNG screenshots.
Each call does one action; the editor process holds the state between calls.

All paths are relative to the repo root (`C:\dev\RZE`). The commands are PowerShell 5.1.

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
cmd /c RZE\AssetCpy.bat
```

| step | why |
|---|---|
| `GenerateSolutions.bat` | Sharpmake generates `.sln`/`.vcxproj`, which are gitignored. **Re-run it after adding or removing any source file**, or the new file won't compile. |
| MSBuild | Builds Debug x64 into `RZE\_build\debug\`. A clean incremental build is fast. |
| `SourceAssetBurner.exe` | Imports `RZE\Assets\3D\**` into `RZE\ProjectData\` (gitignored). Needed once after cloning. Takes about 15 s and writes into `RZE\ProjectData` wherever you run it from. |
| `AssetCpy.bat` | Copies `Assets/`, `Config/` and `ProjectData/` into `_build\debug` and `_build\release` with `xcopy /d`, so only newer files. **The build does not do this.** Only `Game.exe` reads these copies; the editor doesn't (see Gotchas). Re-run it before running the Game after editing assets or shaders. |

## Run (agent path)

```powershell
$d = '.\.claude\skills\run-rze\driver.ps1'
& $d launch -Scene DrawLineTest.scene      # waits for the window title, then 5 s for assets to stream in
& $d shot                                  # -> $env:TEMP\rze-shots\<timestamp>.png
& $d click -X 60 -Y 95                     # select "NeckMech" in the Scene panel
& $d click -X 2303 -Y 254                  # tick "Draw Mesh Bounds" in Component View
& $d click -X 2372 -Y 143; & $d key -Keys '^a45{ENTER}'   # set Rotation Y = 45
& $d shot -Out "$env:TEMP\rze-shots\flow.png"
& $d alive                                 # running pid=... responding=True
& $d quit                                  # kills Editor.exe; unsaved scene edits are discarded
```

**Always Read the screenshot afterwards and look at it.** Coordinates are client-area physical pixels, the same space as the screenshot.

| command | what it does |
|---|---|
| `launch [-Scene X.scene] [-Config debug\|release] [-SettleSeconds 5]` | Starts `Editor.exe -scene X` with the working directory set to `RZE\_build\<config>`. Refuses to start if an editor is already running. |
| `shot [-Out path.png]` | Captures the client area only. Defaults to `$env:TEMP\rze-shots\`. |
| `click -X -Y` | Left click. |
| `drag -X -Y -X2 -Y2` | Left-button drag in 10 steps. Verified on the translate gizmo's X arrow. |
| `key -Keys '...'` | `SendKeys` syntax. `'^a<value>{ENTER}'` replaces an ImGui number field. |
| `info` | Window title and client size. |
| `alive` | `running pid=N responding=True/False` or `not running`. |
| `quit` | Kills the editor and polls until the process is gone. |

Scenes live in `RZE\Assets\Scenes\`:
- `DrawLineTest.scene`: small; NeckMech (5 sub-meshes), M4, and a directional light.
- `Sponza.scene`: heavy; Sponza has 25 sub-meshes. Use `-SettleSeconds 20`.

### Layout reference (maximized, 2560x1369 client)

The editor starts maximized on a 2560-wide monitor. ImGui panels scale with the window, so **re-derive coordinates from a fresh `shot` on any other monitor.**

✓ = clicked through the driver and confirmed. Unmarked = measured from a screenshot only.

| target | client coords |
|---|---|
| Scene panel rows (Camera, DirectionalLight, ...) | x≈60, y = 59 + 18·row (row 0 = Camera). NeckMech = row 2 → (60, 95) ✓; Sponza in Sponza.scene → (51, 95) ✓ |
| Component View: Position X / Y / Z field | (2310 / 2372 / 2435, 102) |
| Component View: Rotation X / Y / Z field | (2310 / 2372 ✓ / 2435, 143) |
| Component View: Scale X / Y / Z field | (2310 / 2372 / 2435, 186) |
| RenderComponent "Draw Mesh Bounds" checkbox | (2303, 254) ✓ |
| RenderComponent "Draw Sub-Mesh Bounds" checkbox | (2303, 278) ✓ |
| DrawLineTest: NeckMech translate gizmo, X arrow (after selecting NeckMech) | drag (1555, 1038) → (1755, 1038) moves Position X ≈ +5.9 ✓ |

The top menu bar shows `RenderCommand MemArena: <N> KB`, which is the render command memory used per frame. Compare it before and after a change to check render-path cost: for example, 600 debug-line vertices add about 14 KB.

## Run (human path)

Open `RZE\RZE.sln` in Visual Studio, set **Editor** as the startup project, and press F5. `RZE\Editor\_project\Editor.args.json` has preset `-scene` arguments.

## Test

There is no unit-test suite. CI (`.github/workflows/Windows.yml`) only builds and burns assets. Verification means building, then driving the editor as above.

## Gotchas

- **Never `ShowWindow(SW_RESTORE)` the editor to focus it.** On a maximized window it un-maximizes it (2560x1369 → 1584x861), the whole ImGui layout reflows, and every coordinate you measured is wrong. The driver only restores when the window is minimized.
- **Debug draws are stripped from Retail builds.** `DebugDrawRenderStage` (debug lines, bounds boxes, the light's direction line, camera frustums) is only added when `RZE_RETAIL` isn't defined, so Debug and Release both show them.
- **The editor reads assets from the source tree, not from `_build\<config>`.** `EditorMain.cpp` sets `EDirectoryContext::Tools`, so `Filepath` resolves paths against the folder above `_build\` (e.g. `RZE\Assets\Shaders\...`). Shader edits take effect on the next editor launch with no `AssetCpy.bat`. This also means the shader in `RZE\Assets` is what runs: to capture a "before" image, revert or disable the change in the source file itself, not just in `_build`. `Game.exe` uses `EDirectoryContext::Runtime` and reads from its own folder (`_build\<config>`), so it does need `AssetCpy.bat`.
- **Large screenshots are downscaled when Read.** A 2560-wide PNG shows at 2000 px with a note to "multiply coordinates by 1.28". Measure positions in the displayed image, then multiply by that factor before passing them to `click`.
- **The window title appears before the scene finishes loading.** Meshes stream in afterwards. `-SettleSeconds` covers this: 5 is enough for DrawLineTest, Sponza needs about 20.
- **`Stop-Process` returns about 2 s before `Editor.exe` actually exits.** The render thread and D3D device are still tearing down. `quit` polls for this; don't `launch` immediately after a bare `Stop-Process`.
- **The light's line in DrawLineTest is off-screen at load.** Move the DirectionalLight onto NeckMech (Position ≈ -22.7, 14, -9) to see it.
- **The camera starts inside Sponza.** Bounds boxes there look like long lines crossing the view, not boxes. Use DrawLineTest to judge box shapes.

## Troubleshooting

- **`'GenerateSolutions.bat' is not recognized as an internal or external command`**: you ran it through `cmd /c "cd /d ...\RZE & call GenerateSolutions.bat"` from PowerShell. That failed in this environment even though the file exists. Use `Push-Location RZE; & .\GenerateSolutions.bat` instead.
- **Clicks land in the wrong place after the first one**: something un-maximized the window. Check `& $d info` for client size. Either re-maximize it or `quit` and `launch` again.
- **`Editor.exe already running. Run: driver.ps1 quit`**: a previous session left one open. Run `& $d quit`.
