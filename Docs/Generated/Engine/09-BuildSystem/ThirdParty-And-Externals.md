# Third-Party Dependencies & Vendored Source

RZE gets its libraries three ways:
- **prebuilt binaries** committed under `ThirdParty/`, which Windows links;
- **system packages** on Linux (`dnf install`), the same libraries built for it;
- **vendored source** compiled as part of the solution: `Externals/` (its own Sharpmake project, see
  [Sharpmake-Project-Graph.md](Sharpmake-Project-Graph.md)) and, on Linux, Optick.

## Per library, per platform

| Library | Purpose | Windows | Linux |
|---|---|---|---|
| SDL3 3.4.18 | The window, input and displays, on both platforms ([Windowing.md](../06-Platform-And-Infrastructure/Windowing.md)) | `ThirdParty/Include/SDL3`, `Lib/x64/SDL3.lib`, `Dll/x64/SDL3.dll` (the official VC build) | `SDL3-devel` |
| Assimp 6.0.5 | 3D model import (`.obj`, `.fbx`, ...), in `SourceAssetBurner`'s `AssimpSourceImporter` | `assimp-vc141-mt` (built from source with the v141 toolset, Release, shared) | `assimp-devel` |
| shaderc | HLSL to SPIR-V for the OpenGL renderer (`GLShaderTranslator`) | not used | `libshaderc-devel` |
| Optick 1.3 | CPU profiler, in the render thread and main loop | prebuilt `OptickCore.lib`/`.dll` | built from `ThirdParty/Include/Optick` (`OptickCore.sharpmake.cs`), with `OPTICK_ENABLE_GPU=0` |
| DirectXTK | DX11 helpers: texture loaders, SimpleMath, effects | `DirectXTK.lib` | not used |
| GLM 0.9.8 | Maths, wrapped by `Utils`' Vector/Matrix/Quaternion types | header-only, vendored | the same vendored headers |
| RapidJSON | JSON: scene files (`GameScene::Serialize`/`Deserialize`), EditorHarness's channel | header-only, vendored | the same |
| SimpleINI | INI parsing, behind `Config`/`EngineConfig` | header-only, vendored | the same |
| STB | Single-header libraries: image loading, PNG writing (`TextureFile`) | header-only, vendored | the same |
| doctest 2.4.11 | Unit tests (`doctest/doctest.h`, MIT licence alongside), for `UnitTests` | header-only, vendored | the same |
| FreeType | Font rasterization | headers | the same headers |

Vendored headers are the same files on both platforms, so both compile identical code. On Linux, `ThirdParty/Include`
is a system include path (`-isystem`), so warnings in it don't fail the `-Werror` build.

Leftovers not used by anything: `ThirdParty/Include/GL` and `glew64` (GLEW, from before the engine settled on DX11;
the OpenGL renderer loads GL through glad instead, below), and three Bullet `vs2010` debug libraries in `Lib/x64`.

## `Externals/`: vendored source, compiled as part of the solution

`Externals/Externals.sharpmake.cs` (static lib, no RZE dependencies). It links SDL3 (`SDL3.lib` / `libSDL3.so`) for
ImGui's SDL backend and everything above it. On Linux it compiles with `-w`: all of it is someone else's code.

- **`Src/imgui/`:** Dear ImGui 1.92.9b (docking) and its community extensions.
  - Core: `imgui.cpp/.h`, `imgui_draw`, `imgui_widgets`, `imgui_tables`, `imgui_demo`.
  - Backends:
    - `imgui_impl_sdl3`, ImGui's platform backend on both platforms (`Windowing/ImGuiPlatform`);
    - `imgui_impl_dx11` on Windows and `imgui_impl_opengl3` on Linux, the renderer backends behind
      `IImGuiRenderer` ([Low-Level-Renderer-Module.md](../04-Rendering/Low-Level-Renderer-Module.md)).

    Each renderer backend is left out of the other platform's build by name, rather than moved into a token folder,
    so ImGui's own files stay as released.
  - Extensions: **ImGuizmo** (3D transform gizmos, in `SceneViewPanel`), **ImCurveEdit**/**ImGradient** (curve and
    gradient editors), **ImSequencer**, **GraphEditor**, **ImZoomSlider**.
- **`Src/OpenGL/`**, compiled on Linux only (the `OpenGL` folder token):
  - `glad/`: the GL 4.5 core loader with `KHR_debug`, generated once by glad2 and committed. GLEW doesn't work under
    EGL on Wayland, which SDL uses there.
  - `SPIRV-Cross/`: the SPIR-V to GLSL compiler (tag `vulkan-sdk-1.4.341.0`, Apache-2.0), which turns shaderc's
    output into GLSL 4.50. It's not packaged for Fedora.

## `Files/`: original download archives (not built)

`Files/` holds the archives many of the above were unpacked from:
- `assimp-6.0.5-win64-vc141-binaries.zip` (the headers, import library, DLL and LICENSE of the build above);
- `freetype-2.8.tar.gz`, `glew-2.1.0-win32.zip`, `rapidjson-master.zip`, `stb-03262022.zip`, `imgui-docking.zip`,
  `ImGuizmo-1.83.zip`, `Optick_1.3.0.zip`;
- `Brofiler-1.1.2.zip` (Optick's predecessor, not integrated);
- `bullet3-master.zip` (Bullet Physics, not wired into anything);
- `RenderDoc_1.2_64.msi` and `Setup/vcredist_x64.exe`;
- sample-art archives (`nanosuit.rar`, `PanzerTank.rar`, `FW190_Plane.zip`, `m4a1.rar`, a neck-mech-walker rar),
  presumably unpacked into `Assets/3D`.

**SDL3's VC archive isn't here**, although the repo's convention is to keep one per vendored binary: its files were
committed straight into `ThirdParty/`.

## `Tools/`: bundled binaries (not source)

- `Tools/RZEHub.exe`, Windows only: a launcher GUI whose source isn't in this repo. It runs `SourceAssetBurner` for
  you ("Burn Assets") and expects the depot at `C:\dev\RZE`. Its "Asset Copy" button ran `AssetCpy.bat`, which was
  removed when the Game started reading the source tree, so that button no longer works.
- `Tools/Assimp/`: `assimp.dll`, `assimp.exe`, `assimp_viewer.exe`.
- `Tools/Optick/Optick.exe`: the Optick profiler's viewer, Windows only. On Linux, `driver.py optick` takes a capture
  over Optick's network protocol instead (the `run-rze` skill, [EditorHarness-And-Drivers.md](../07-Editor/EditorHarness-And-Drivers.md)).
- `Tools/Sharpmake/`: the Sharpmake application (`Sharpmake.Application.exe`/`.dll`, `Sharpmake.dll`,
  `Sharpmake.Generators.dll`, `Sharpmake.CommonPlatforms.dll`, Roslyn dependencies), which `GenerateSolutions.bat` and
  `GenerateSolutions.sh` run.

## Key locations

- `RZE/ThirdParty/Include/`, `Lib/x64/`, `Dll/x64/`, `OptickCore.sharpmake.cs`
- `RZE/Externals/Externals.sharpmake.cs`, `Src/imgui/`, `Src/OpenGL/`
- `Files/`, `Tools/`
