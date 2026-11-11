# Engine Graphics Layer: `RenderEngine`

`RenderEngine` (`Engine\Src\Graphics\RenderEngine.h/.cpp`) is the engine's rendering coordinator. It's reached through `RZE().GetRenderEngine()`, and it sits between gameplay code and the low-level renderer:

```mermaid
%%{init: {'flowchart': {'subGraphTitleMargin': {'top': 6, 'bottom': 10}}, 'themeCSS': '.labelBkg, .edgeLabel { background-color: var(--background-primary) !important; } .messageText, .labelText, .loopText { paint-order: stroke; stroke: var(--background-primary); stroke-width: 12px; stroke-linejoin: round; }'}}%%
flowchart TD
    subgraph Game["Game objects (Engine/Src/Game)"]
        RC["RenderComponent"]
        DL["LightComponent"]
        RPC["ReflectionProbeComponent"]
        CAM["CameraComponent /<br/>EditorCameraComponent"]
    end
    SD["RenderScene<br/>one per GameScene: render objects,<br/>lights, reflection probes,<br/>debug lines, revision"]
    MV["RenderEngine's main RenderView<br/>camera, viewport, target"]
    PL["RenderEngine's RenderPipeline<br/>+ render stages"]
    LL["Rendering::Renderer<br/>(Modules/Rendering, render thread)"]

    RC -->|"CreateRenderObject / SetTransform"| SD
    DL -->|"CreateLightObject / SetPosition, SetDirection"| SD
    RPC -->|"CreateReflectionProbe / SetPosition, SetExtents"| SD
    CAM -->|"fills Camera each frame"| MV
    SD --> PL
    MV --> PL
    PL -->|"draw commands"| LL
```

The render-side scene is deliberately separate from `GameScene`: a `RenderObject` knows its mesh, transform and bounds, plus an opaque pick ID, but nothing about the `GameObject` that owns it.

## Scene data

A **`RenderScene`** (`Engine\Src\Graphics\RenderScene.h/.cpp`) is what one scene draws. **Each `GameScene` owns one** (`GameScene::GetRenderScene()`), and its components put their render objects and lights in it. Any view can render any scene, so a scene can be drawn without being the one on screen, e.g. a [`HeadlessScene`](../03-SceneAndGameObjects/Scene-And-GameObject-Model.md#headlessscene-a-scene-rendered-off-screen) for a thumbnail.

| Holds | Notes |
|---|---|
| Render objects, lights | `CreateRenderObject(staticMesh)` / `DestroyRenderObject`, `CreateLightObject(type)` / `DestroyLightObject`, `ClearObjects()` (render objects only). `GetDirectionalLight()` is the first directional light, or null. |
| Reflection probes | `CreateReflectionProbe()` / `DestroyReflectionProbe`, `GetReflectionProbes()` in the order they were made, which is the order `ReflectionProbeStage` gives them its 16 slots (below). |
| Debug lines | `DrawLine` / `DrawWireBox` / `DrawWireSphere`, kept until `ClearDebugLines()`. `RenderEngine::Render` clears the main scene's once it has drawn them. |
| `GetRevision()` | A cheap "did anything change?" counter. It goes up when a render object is added, removed, moved or given a new mesh, when a light is added, removed, or changed in a way that moves its shadows (its type, a directional light's direction, a point light's position or range; colour and strength don't count), or when a reflection probe is added, removed, moved or resized. `ShadowRenderStage` skips checking the scene's shadow maps when it hasn't changed. |
| `GetObjectsRevision()` | The revision of the last render object change alone (lights don't move it). The sun's shadow map is out of date once it's newer than the revision the map was rendered at. |
| `HaveObjectsChangedWithin(region, sinceRevision)` | Whether a render object was added, removed, moved or given a new mesh within a world-space box after a revision, counting both where it was and where it is. Backed by a `SceneChangeLog` (below). A point light's shadow cube asks it about the box its six faces see. |
| `GetBounds()` | The box around every render object, kept until one is added, removed, moved or given a new mesh, then rebuilt from the objects' boxes when next asked. |
| Persistent stage state | What stages keep for this scene across frames, whichever view renders it: **each scene's own light buffer and shadow maps, and its irradiance and reflection probes**. `ReleaseStageState()` drops them; they're made again the next time the scene is rendered. |

**Where objects changed (`SceneChangeLog`).** Each render object change is logged as world-space boxes stamped with the new revision: a move or a new mesh logs where the object was and where it is as two boxes, so an object that jumps across the scene doesn't mark everything between; a new object logs where it is and a destroyed one where it was. `ClearObjects` counts as a change everywhere. The log keeps the latest 256 changes (`SceneChangeLog::k_capacity`). Asking about a revision from before them answers that it changed, so the answer can be true for a change elsewhere once many objects have moved, but never false for one within the box.

### Handles

`CreateRenderObject`, `CreateLightObject` and `CreateReflectionProbe` return a **`RenderObjectPtr`** / **`LightObjectPtr`** / **`ReflectionProbeObjectPtr`** (`Graphics\GraphicsDefines.h`: `UniqueLiveObjectPtr` from `Utils\Src\Utils\LiveObjectPtr.h`, with the `RenderSceneLifetime` policy). Whoever made the object keeps the handle and destroys the object through it (`DestroyRenderObject(handle)` empties it).

- **Move-only**, so each object has one holder (`RenderComponent`, `LightComponent`, `ReflectionProbeComponent`). Code that only looks at an object borrows the holder's handle by reference, e.g. `RenderComponent::GetRenderObject()` returns `const RenderObjectPtr&`.
- **Lifetime-checked**, like `GameObjectPtr`. Each object takes a 64-bit ID that's never handed out twice (`RenderSceneLifetime`: one `LiveIDAllocator` shared by render objects, lights and reflection probes, made on first use and never destroyed; see [Handles](../03-SceneAndGameObjects/Scene-And-GameObject-Model.md#handles-gameobjectptr-and-gameobjectcomponentptrt)). A handle compares equal to `nullptr` once its object is gone, however it went: destroyed through the handle, `ClearObjects`, or the scene being destroyed. Debug builds assert if a dead handle is dereferenced, and destroying through a dead handle just empties it.

`RenderEngine` doesn't own a scene. `SetMainScene(scene)` says which one the main view shows; `RZE_Engine` sets it to the active `GameScene`'s. `RenderEngine::DrawLine` / `DrawWireBox` draw into the main scene, for tools that draw over the main view (the editor's grid and gizmos).

### `RenderObject`

| Holds | Notes |
|---|---|
| `StaticMeshInstance` | What it draws: a shared mesh with this object's material slots ([Materials-Meshes-Shaders.md](Materials-Meshes-Shaders.md#shared-meshes-per-object-instances)). `SetMeshInstance()` swaps it, keeping the render object; null draws nothing. |
| `GraphicsLayouts::ObjectBufferLayout { Transform, InvTransform }`, uploaded as the vertex shaders' `ObjectBuffer` | `SetTransform()` is called every frame by `RenderComponent`, but returns early if the matrix hasn't changed. |
| Draws | `GetDraws()`: one `MeshDraw` per placed sub-mesh, with its geometry, the material slot it draws with, its world transform, and 8 world-space corners plus the box around them. |
| Bounds | `GetLocalBounds()`: the mesh's box in the object's own space, which only changes with the mesh. `GetBounds()`: that box placed in the world as an axis-aligned `BoundingBox`. The world boxes and the draws are **worked out when asked** after the object moves or changes mesh (`MarkChanged()`), not on every move. Used for frustum culling, shadow fitting, framing thumbnails, the Editor's marquee selection and its bounds boxes. |
| Pick ID | `SetPickID()` / `GetPickID()`, a U64. What `PickRenderStage` writes for this object's pixels. `RenderComponent` sets it to its owner's `GameObjectID + 1`, so 0 means "nothing pickable". |

### `LightObject`

A light of any type. It holds the data every type shares, and its **type** (`ELightType`) decides which of it is read and which path it takes through lighting and shadows:

| | `Directional` | `Point` |
|---|---|---|
| Reads | `direction` (normalized, the way light travels) | `position`, `range` (metres to where it fades to nothing) |
| Also reads | `colour` (sRGB), `strength` | `colour` (sRGB), `strength` |
| Falloff | None: the same everywhere | Inverse square over distance as a fraction of the range, windowed to exactly 0 at the range (below) |
| Shadows | The first one: a 2D map fitted around the scene | The first 8: a cube each, in a cube map array |

It has **no GPU resources of its own**. `ShadowRenderStage` puts every light of a scene into one buffer each frame (see [Render-Stages.md](Render-Stages.md#shadowrenderstage)). `GetShadowRevision()` is the scene revision when the light was made or its shadows last moved (the same changes that move `GetRevision`), so its shadow map is out of date once that's newer than the revision the map was rendered at. `SetType` can change a light's type in place, which is how the inspector's Type drop-down works without remaking it. The `ELightType` values are what the shaders branch on (`LIGHT_TYPE_*`), so new types go at the end.

```mermaid
%%{init: {'flowchart': {'subGraphTitleMargin': {'top': 6, 'bottom': 10}}, 'themeCSS': '.labelBkg, .edgeLabel { background-color: var(--background-primary) !important; } .messageText, .labelText, .loopText { paint-order: stroke; stroke: var(--background-primary); stroke-width: 12px; stroke-linejoin: round; }'}}%%
flowchart TD
    LC["LightComponent<br/>Type, Colour, Strength, Range"]
    LO["LightObject in the RenderScene<br/>type + shared data, no GPU resources"]
    SRS(["ShadowRenderStage"])
    SLD[/"SceneLightingData<br/>LightBuffer, ShadowBuffer,<br/>sun's map, point light cubes"/]
    FRS(["ForwardRenderStage"])
    SH["LightResources.hlsli:<br/>per light, the type picks<br/>direction, falloff and<br/>shadow lookup<br/>Lighting.hlsli: the BRDF"]

    LC -->|"Update: position, direction from the transform"| LO
    LO --> SRS
    SRS -->|"lights every frame,<br/>each map when what<br/>it shows changes"| SLD
    SLD --> FRS
    FRS --> SH
```

**Point light falloff.** `strength × f(distance / range)`, with `f(x) = (1 − x⁴)² / (1 + 8x²)`: inverse square, but over the fraction of the range rather than metres, so a longer range reaches further instead of only moving where a physically faded light is cut off. Strength is the brightness right next to the light. `LightObject::CalculatePointFalloff` is the same function on the CPU, for the editor's gizmo; its constant (`k_pointFalloffSharpness`) mirrors `PointFalloffSharpness` in `Common/LightResources.hlsli`.

**Where a point light outshines the ambient light.** `LightObject::CalculateOutshinesAmbientRatio(strength, colour)` is the fraction of the range within which the light lights a surface facing it more than the ambient light does (the sky and ground halfway, as a wall gets them). It grows with strength, up to 1, and is 0 for a light dimmer than the ambient light even next to it. The editor draws it as the inner range sphere. The ambient colours (`k_ambientSky`, `k_ambientGround`) mirror `AmbientSky` and `AmbientGround` in `Common/LightResources.hlsli`, the sky's colours, which only light surfaces directly where no irradiance probe has been lit.

### `ReflectionProbeObject`

A point that captures its surroundings for glossy surfaces to reflect, and the box it's used in: centred on it, along the world's axes, sized to the room so reflections can be projected onto its sides. `ReflectionProbeComponent` (Size, Blend Distance) places one at its object's position; the editor shows it as a shiny-ball icon with its box. Like a light, it has no GPU resources of its own; `ReflectionProbeStage` keeps a scene's together ([Render-Stages.md](Render-Stages.md#reflectionprobestage)). It's given the scene's revision counter when it's made, and moving or resizing it advances it, so its captures can tell they're out of date (`GetRevision()`).

## Cube maps (`CubeMap.h/.cpp`)

The cube map convention, for anything that renders into or samples a cube. `ECubeFace { Right, Left, Up, Down, Forward, Back }` names the faces by the world direction each looks in, in the order a cube texture stores them (slices 0–5). `CubeMap::GetFaceForward(face)` / `GetFaceUp(face)`, `CreateFaceView(position, face)` (the view through the face, built with `Matrix4x4::CreateViewMatrix`), and `ToSampleDirection(worldDirection)`. D3D's cube is laid out for a left-handed world with +Z forward and ours is right-handed with −Z forward, so the cube's +Z face is our **Forward**, and sampling flips Z. With that, `CubeMap::CreateFaceView` and a 90°, square projection render each face exactly as sampling reads it, unmirrored, so triangles keep their winding (tested over the whole sphere against D3D's face table in `CubeMapTests.cpp`). `ToCubeSampleDirection` in `Common/LightResources.hlsli` mirrors `ToSampleDirection`. Used for point light shadows and reflection probes. It's in the Engine, not `Utils/Math`, because what it encodes is D3D's cube layout, not maths: see [World-Conventions.md](../02-Architecture/World-Conventions.md). The Rendering module only knows a cube texture as six slices per cube (`GFXTextureBufferParams::bIsCubeMap`), in this order.

## Views

A **`RenderView`** is one point of view to render the scene from. Its owner fills in the inputs; render stages read them.

```cpp
class RenderView
{
public:
    RenderCamera Camera;                     // ClipSpace (projection * view), Position, then Projection alone;
                                             // the shaders' CameraBuffer reads up to Position
    Vector2D ViewportSize;
    Rendering::RenderTargetTexture* Target;  // owned by whoever owns the view
    bool AutoExposure = true;                // off for a fixed exposure of 1, for lighting whose brightness is known
    float ExposureCompensation;              // EV on top of the exposure
    AntiAliasingMode AntiAliasing;           // after tonemapping: Method (Off, FXAA, SMAA) and Quality
                                             // (Low, Medium, High, Ultra); SMAA High by default
    EAmbientOcclusion AmbientOcclusion = Medium; // GTAO: Off, Low, Medium or High
    bool GlobalIllumination = true;          // indirect light from the scene's probes; off for the hemisphere ambient
    ERenderViewDebug Debug = Lit;            // Lit, or one part of the lighting alone: AmbientOcclusion, IndirectDiffuse
    ERenderViewLighting Lighting = Scene;    // Scene: the scene's light and sky. Uniform: even, ignoring the light
    bool DrawsDebug = true;                  // debug lines; off for clean captures such as thumbnails
    RenderFrameRequests Frame;               // for the next render only, cleared after it (ClearFrameRequests):
                                             //   Pick, the Editor's picking (clicks, drops), and Outlines,
                                             //   lines around objects (the Editor's selection; OutlineRenderStage)
    // + a frame blackboard and a persistent blackboard (see Render-Stages.md)
};
```

```mermaid
%%{init: {'flowchart': {'subGraphTitleMargin': {'top': 6, 'bottom': 10}}, 'themeCSS': '.labelBkg, .edgeLabel { background-color: var(--background-primary) !important; } .messageText, .labelText, .loopText { paint-order: stroke; stroke: var(--background-primary); stroke-width: 12px; stroke-linejoin: round; }'}}%%
flowchart LR
    subgraph Main["Main view (one, owned by RenderEngine)"]
        direction TB
        M1["Camera: EditorCameraComponent<br/>or the active CameraComponent"]
        M2["Target: the app's render target"]
        M3["Rendered by RenderEngine::Render()"]
    end
    subgraph Sec["Secondary views (any number)"]
        direction TB
        S1["e.g. a CameraComponent's preview,<br/>or a ViewCapture"]
        S2["Target: the owner's own texture"]
        S3["Rendered by RenderSecondaryView()<br/>with any RenderScene"]
    end
```

- `GetMainView()` returns the main view. The app sets its `Target` and `ViewportSize` (the Editor sizes it to the scene view panel); the active scene's main camera fills in `Camera` every frame. Engine.ini's `[Rendering]` sets its `AntiAliasing`, `AmbientOcclusion`, `GlobalIllumination` and `AutoExposure` at start-up, and the Editor's Graphics settings change them, and `Debug`, while it runs.
- A camera that renders a view fills in `RenderCamera::Projection` as well as `ClipSpace` (`CameraComponent`, `EditorCameraComponent`, `MeshFraming`). A view without a perspective projection there gets no ambient occlusion.
- `RenderSecondaryView(name, view, scene)` renders a scene into another view as part of the current frame. `CameraComponent` uses this for the "Camera View" preview it draws in the Editor's inspector (`CameraComponent::RenderPreview`, called by the camera's `Preview` property each frame the inspector shows it), so a preview is only rendered while that camera is selected. Stages marked `MainOnly` (picking, ImGui, the final blit) skip secondary views.

## Capturing a view to a file

`ViewCapture` (`Engine\Src\Graphics\ViewCapture.h/.cpp`) renders a scene from a camera into an offscreen target of its own, at a size fixed when it's made, and saves it as a PNG. The editor's thumbnails use a 512×512 one; it's in the Engine so the game could take screenshots the same way.

```mermaid
%%{init: {'flowchart': {'subGraphTitleMargin': {'top': 6, 'bottom': 10}}, 'themeCSS': '.labelBkg, .edgeLabel { background-color: var(--background-primary) !important; } .messageText, .labelText, .loopText { paint-order: stroke; stroke: var(--background-primary); stroke-width: 12px; stroke-linejoin: round; }'}}%%
flowchart TD
    Cap["ViewCapture::Capture(scene, camera, settings, file)"] --> Sec["RenderSecondaryView<br/>its own view, DrawsDebug off"]
    Sec --> Save["TextureFile::SaveToPNG(target, file)"]
    Save --> Read["Renderer::ReadTexture<br/>queued straight after the draw"]
    Read --> Poll["background operation polls<br/>TextureReadback::IsReady"]
    Poll --> Write["worker: WritePNG<br/>to file.tmp, then moved over file"]
```

- `Capture` returns the `AsyncOperation` that writes the file. The image is read back from the commands queued so far, so another capture can follow in the same frame.
- **No debug lines** (grid, frustums, bounds boxes): its view has `DrawsDebug` off.
- Its view has `GlobalIllumination` off: one frame is too few for probes to capture and settle, so a capture lights with the hemisphere ambient, and the scene doesn't get probe textures it would only use once.
- `ViewCaptureSettings` sets the view's `Lighting`, `AutoExposure` and `ExposureCompensation` for each capture. The defaults light and expose like any other view. Mesh thumbnails use `Uniform` lighting with auto-exposure off ([Thumbnails-And-File-Browser.md](../07-Editor/Thumbnails-And-File-Browser.md#a-run)).
- `TextureFile` (`Engine\Src\Graphics\TextureFile.h/.cpp`) wraps `stb_image_write`. `WritePNG(file, width, height, rgba)` is safe from any thread. It creates the folder, writes the PNG beside the file and moves it over the file (`File::MoveReplacing`), so readers only ever see a whole image. `SaveToPNG(texture, file)` is the GPU-to-file path above. Its operation is `Background | Hidden`, since whoever asked for the capture reports on it.

## Frame lifecycle

```mermaid
%%{init: {'flowchart': {'subGraphTitleMargin': {'top': 6, 'bottom': 10}}, 'themeCSS': '.labelBkg, .edgeLabel { background-color: var(--background-primary) !important; } .messageText, .labelText, .loopText { paint-order: stroke; stroke: var(--background-primary); stroke-width: 12px; stroke-linejoin: round; }'}}%%
sequenceDiagram
    participant Main as Engine frame loop
    participant RE as RenderEngine
    participant PL as RenderPipeline
    participant R as Rendering::Renderer
    participant RT as Render thread

    Note over Main,RE: game objects update<br/>(transforms, camera, debug lines)
    Main->>RE: Render("Main Render")
    RE->>R: BeginFrame
    RE->>PL: Render(main view, main scene, frame index)
    PL->>R: each stage queues draw commands
    RE->>RE: next frame index, then clear the main scene's<br/>debug lines and the pick request
    RE->>R: EndFrame
    Main->>RE: Finish()
    RE->>R: DevicePresent()
    R->>RT: hand over this frame's commands
    Note over R,RT: runs them on the GPU<br/>while the next frame is being prepared
```

Nothing touches D3D11 on the game thread; every `Renderer::` call just queues a command for the render thread (see [Low-Level-Renderer-Module.md](Low-Level-Renderer-Module.md)).

Every render gets the frame's index (`RenderContext::FrameIndex`), the same for each view in the frame: secondary views render before the main view, and the index moves on after it. Stages use it to do a scene's work once a frame, whichever view draws it first (the probes).

## API summary

| Area | Calls |
|---|---|
| Lifetime | `Initialize(windowHandle)` (also adds the engine's stages), `Shutdown()`, `ReleaseRenderStages()` (must run before `ResourceHandler::ShutDown()`) |
| Per frame | `Render(frameName)` (the main view, of the main scene), `Finish()`, `RenderSecondaryView(name, view, scene)` |
| Scene | `SetMainScene(scene)` / `GetMainScene()`. Objects and lights are made in a [`RenderScene`](#scene-data). |
| Debug drawing | Into the main scene: `DrawLine(start, end, colour, alpha = 1, fade = {})`, `DrawWireBox(box, transform, colour, alpha = 1)`, `DrawWireSphere(centre, radius, colour, alpha = 1, occludedAlpha = 0)` (three circles, one around each world axis). `alpha` makes lines semi-transparent; `fade` (`DebugLineFade { centre, start, end }`) fades a line out with each pixel's distance from `centre`, between the `start` and `end` radii (`end` = 0: no fade). The editor grid uses both. `occludedAlpha` (`DebugLine::occludedAlpha`) is how opaque a line's parts behind the scene are drawn, so e.g. a light's range still shows through the walls it passes behind; 0 hides them, as for every other line. |
| Stages | `AddRenderStage<T>(args...)` (see [Render-Stages.md](Render-Stages.md)) |
| Stats | `GetStats()` (`RenderStats`, main thread only): per frame, the time recording every view's render stages, each stage's part of it (`RenderStageTime`, from `RenderPipeline::TakeStageTimes()`), and draw calls not counting the UI's. Averaged over a `StatsWindow`; a frame runs from one `Render()` to the next, and the window is updated as `Render()` starts, before recording. The Editor's status bar shows them. |
| Views & canvas | `GetMainView()`, `ResizeCanvas(size)`, `GetCanvasSize()` |
| Probe budgets | `GetGlobalIlluminationSettings()` / `SetGlobalIlluminationSettings(settings)`: `GlobalIlluminationSettings` (`Graphics/GlobalIllumination.h`), the probe count and the capture and relight budgets for every scene, from Engine.ini's `[Rendering]` `ProbeTargetCount`, `ProbeCaptureDrawBudget` and `ProbeRelightBudget`. `IrradianceProbeStage` reads them each frame. |

Both object containers erase with swap-and-pop (`ContainerUtils::VectorEraseBack`), since their order doesn't matter.

## Key files

- `Engine\Src\Graphics\RenderEngine.h/.cpp` — `RenderEngine`
- `Engine\Src\Graphics\RenderScene.h/.cpp` — `RenderScene` (and `RenderSceneLifetime`'s ID allocator)
- `Engine\Src\Graphics\RenderObject.h/.cpp` — `RenderObject`
- `Engine\Src\Graphics\LightObject.h/.cpp` — `LightObject`, `ELightType`
- `Engine\Src\Graphics\ReflectionProbeObject.h/.cpp` — `ReflectionProbeObject`, and `Engine\Src\Game\World\GameObjectComponents\ReflectionProbeComponent.h/.cpp`, the component that places one
- `Engine\Src\Graphics\GlobalIllumination.h` — `GlobalIlluminationSettings`
- `Engine\Src\Graphics\DebugLine.h` — `DebugLine`, `DebugLineFade`
- `Engine\Src\Graphics\SceneChangeLog.h/.cpp` — where a scene's render objects changed, by revision
- `Engine\Src\Game\World\GameObjectComponents\LightComponent.h/.cpp` — the component that drives a `LightObject` ([Built-In-Components.md](../03-SceneAndGameObjects/Built-In-Components.md#lightcomponent))
- `Engine\Src\Graphics\RenderView.h` — `RenderView`, `RenderCamera`, `RenderFrameRequests`, `RenderPickRequest`, `RenderOutline`, `ERenderViewDebug`
- `Engine\Src\Graphics\ViewCapture.h/.cpp`, `TextureFile.h/.cpp` — capturing a view to a PNG
- `Utils\Src\Utils\Math\Frustum.h/.cpp` — the six-plane view frustum used for culling and marquee selection (see [Utils-Library-Reference.md](../06-Platform-And-Infrastructure/Utils-Library-Reference.md))
- `Engine\Src\Graphics\CubeMap.h/.cpp` — `ECubeFace` and the cube map convention
- `Engine\Src\Graphics\GraphicsDefines.h` — `RenderSceneLifetime` and the `RenderObjectPtr` / `LightObjectPtr` / `ReflectionProbeObjectPtr` aliases, move-only and lifetime-checked
- `Engine\Src\Graphics\GraphicsLayouts.h` — the shader buffer layouts the render stages share: lights and shadows (`GraphicsLayouts::LightLayout`, `GraphicsLayouts::ShadowBufferLayout`, ...), an object's matrices (`ObjectBufferLayout`) and a material's parameters (`MaterialBufferLayout`) (see [Render-Stages.md](Render-Stages.md#shadowrenderstage))
