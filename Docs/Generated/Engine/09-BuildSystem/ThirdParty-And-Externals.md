# Third-Party Dependencies & Vendored Source

RZE distinguishes between **prebuilt vendored binaries** (`ThirdParty\`) and **vendored source compiled as part of the solution** (`Externals\`, its own Sharpmake project — see [Sharpmake-Project-Graph.md](Sharpmake-Project-Graph.md)).

## `ThirdParty\` — prebuilt libraries (headers in `Include\`, static libs in `Lib\x64`, runtime DLLs in `Dll\x64`)

| Library | Purpose |
|---|---|
| Assimp | 3D model import (`.obj`/`.fbx`/etc.) — used by `SourceAssetBurner`'s `AssimpSourceImporter` |
| DirectXTK (DirectX Tool Kit) | Audio, SpriteBatch, SimpleMath, texture loaders, effects |
| FreeType | Font rasterization |
| GL | OpenGL headers (constants/loader — DX11 is the active renderer backend, not OpenGL) |
| GLM | OpenGL Mathematics — the underlying math library wrapped by `Utils`' Vector/Matrix/Quaternion types |
| imGUI | Header-only reference here; the actual ImGui *source* is compiled via `Externals` (see below) |
| Optick | CPU/GPU profiler (`OptickCore.dll`), integrated into the render thread and main loop |
| RapidJSON | JSON parsing — scene serialization (`GameScene::Serialize`/`Deserialize`) |
| SimpleINI | INI parsing — backs the `Config`/`EngineConfig` system |
| STB | Single-header libs (e.g. image loading) |

Raw DLLs also present: `OptickCore.dll`, `assimp.dll`, `glew64.dll` (GLEW — OpenGL Extension Wrangler, likely a leftover from before the engine settled on DX11).

## `Externals\` — vendored source, compiled from scratch as part of the solution

`Externals\Externals.sharpmake.cs` (static Lib, zero RZE dependencies). Currently just Dear ImGui + its community extension widgets, all under `Externals\Src\imgui\`:
- Core: `imgui.cpp/.h`, `imgui_draw`, `imgui_widgets`, `imgui_tables`, `imgui_demo`
- Backends: `imgui_impl_win32`, `imgui_impl_dx11` (used directly by `Modules\Rendering\RenderThread::InitializeImGui()`)
- Extensions: **ImGuizmo** (3D transform gizmos — used by `SceneViewPanel`), **ImCurveEdit**/**ImGradient** (curve/gradient editors), **ImSequencer**, **GraphEditor**, **ImZoomSlider**

## `Files\` — original download archives (not built, historical reference)

`C:\dev\RZE\Files\` holds the original source archives many of the above were unpacked from: `assimp-3.1.1-win-binaries.zip`, `freetype-2.8.tar.gz`, `glew-2.1.0-win32.zip`, `rapidjson-master.zip`, an `stb` zip, `imgui-docking.zip`, `ImGuizmo-1.83.zip`, `Optick_1.3.0.zip`, `Brofiler-1.1.2.zip` (Optick's predecessor profiler — not currently integrated), `bullet3-master.zip` (Bullet Physics — **not** currently wired into `ThirdParty`/`Externals`; likely aspirational/exploratory, not an active dependency), `RenderDoc_1.2_64.msi`, and `Setup\vcredist_x64.exe`. Also sample-art archives (`nanosuit.rar`, `PanzerTank.rar`, `FW190_Plane.zip`, `m4a1.rar`, a neck-mech-walker rar) that were presumably unpacked into `Assets\3D`.

## `Tools\` — bundled binaries (not source)

- `Tools\RZEHub.exe` — a custom launcher/dashboard GUI (no source found in this repo — likely maintained separately). Per the README, the intended workflow is: run `RZEHub.exe` → open and build the generated solution → click "Burn Assets" then "Asset Copy" in RZE Hub → start the Editor or Game project. RZEHub appears to be a GUI front-end that invokes `SourceAssetBurner.exe` and `AssetCpy.bat` for you.
- `Tools\Assimp\` — `assimp.dll`, `assimp.exe`, `assimp_viewer.exe`.
- `Tools\Optick\Optick.exe` — the Optick profiler UI.
- `Tools\Sharpmake\` — the full Sharpmake application (`Sharpmake.Application.exe`, `Sharpmake.dll`, `Sharpmake.Generators.dll`, `Sharpmake.CommonPlatforms.dll`, Roslyn/CodeAnalysis dependencies, plus a macOS variant) — this is what `GenerateSolutions.bat` actually invokes.

## Key locations

- `RZE\ThirdParty\Include\`, `Lib\x64\`, `Dll\x64\`
- `RZE\Externals\Externals.sharpmake.cs`, `Src\imgui\`
- `C:\dev\RZE\Files\`
- `C:\dev\RZE\Tools\`
