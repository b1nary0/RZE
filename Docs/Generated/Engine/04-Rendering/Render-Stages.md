# Render Pipeline & Stages

A frame is drawn by a handful of **render stages**: small, independent passes such as "draw the shadow map" or "tonemap the image". Stages never call each other. Each one declares what data it **writes**, **reads** or **modifies**, and the `RenderPipeline` works out the order from those declarations.

> **In one sentence:** stages hand each other typed data packets ("contracts") through a per-view blackboard, and the pipeline sorts the stages so every packet is written before anyone reads it.

## A frame at a glance

Rounded boxes are stages, and the parallelograms between them are the data contracts they pass along.

```mermaid
%%{init: {'flowchart': {'subGraphTitleMargin': {'top': 6, 'bottom': 10}}, 'themeCSS': '.labelBkg, .edgeLabel { background-color: var(--background-primary) !important; } .messageText, .labelText, .loopText { paint-order: stroke; stroke: var(--background-primary); stroke-width: 5px; stroke-linejoin: round; }'}}%%
flowchart TD
    Shadow(["ShadowRenderStage"]) --> SMD[/"SceneLightingData<br/>lights + shadow maps"/]
    Prepass(["DepthPrepassStage"]) --> SDD[/"SceneDepthData<br/>depth"/]
    SDD --> AO(["AmbientOcclusionStage"])
    AO --> AOD[/"AmbientOcclusionData<br/>visibility"/]
    SMD --> Irr(["IrradianceProbeStage"])
    Irr --> SID[/"SceneIrradianceData<br/>probe light + visibility"/]
    SID --> Refl(["ReflectionProbeStage"])
    Refl --> SRD[/"SceneReflectionData<br/>probe cubes"/]
    SMD --> Forward(["ForwardRenderStage"])
    SDD --> Forward
    AOD --> Forward
    SID --> Forward
    SRD --> Forward
    Forward --> SCD[/"SceneColourData<br/>HDR colour + depth"/]
    SCD --> Post(["PostProcessRenderStage"])
    Post --> DCD[/"DisplayColourData<br/>8-bit colour + depth"/]
    DCD --> Debug(["DebugDrawRenderStage<br/>(modifies)"])
    Debug --> DCD2[/"DisplayColourData<br/>+ debug lines"/]
    DCD2 --> ImGui(["ImGuiRenderStage"])
    DCD2 --> Final(["FinalRenderTargetStage<br/>(Game only)"])
    Pick(["PickRenderStage<br/>(only on a click or drop)"])
```

`ReflectionProbeStage` also reads `SceneLightingData`, and `PostProcessRenderStage` reads `AmbientOcclusionData` for its debug view; those edges are left out to keep the picture readable. `PickRenderStage` stands apart because it uses no contracts. It renders from the scene alone and hands its result straight back to the Editor (see [Picking](#pickrenderstage)).

The order the pipeline settles on is logged when it compiles, e.g. in the Editor:

```
Render pipeline: ShadowRenderStage -> DepthPrepassStage -> AmbientOcclusionStage -> IrradianceProbeStage -> ReflectionProbeStage -> ForwardRenderStage -> PostProcessRenderStage -> PickRenderStage -> DebugDrawRenderStage -> ImGuiRenderStage
```

## The pieces

```mermaid
%%{init: {'flowchart': {'subGraphTitleMargin': {'top': 6, 'bottom': 10}}, 'themeCSS': '.labelBkg, .edgeLabel { background-color: var(--background-primary) !important; } .messageText, .labelText, .loopText { paint-order: stroke; stroke: var(--background-primary); stroke-width: 5px; stroke-linejoin: round; }'}}%%
flowchart TD
    RE["RenderEngine"] -->|owns| RP["RenderPipeline"]
    RE -->|owns| MV
    RP -->|"owns, in add order"| Stages["IRenderStage x N"]
    RP -->|"Render(view, scene)"| Ctx["RenderContext<br/>(one per view render)"]
    Ctx -->|"View, Scene, DeltaTime,<br/>FrameIndex"| Stages
    subgraph MV["main RenderView"]
        Frame["frame blackboard<br/>(cleared every render)"]
        Persist["persistent blackboard<br/>(lives as long as the view)"]
    end
    Stages -.->|"ports: Get / Publish"| Frame
    Stages -.->|"Persistent()"| Persist
```

| Piece | File | What it is |
|---|---|---|
| `IRenderStage` | `Graphics/RenderStage.h` | A pass: `Initialize()`, `Setup(builder)` to declare its data, `Render(context)` to draw. The header has a step-by-step guide to writing one. |
| Data contract | `Graphics/RenderData/*.h` | A small struct of handles and values deriving `RenderStageData<T>`, e.g. `SceneLightingData { LightBuffer, ShadowBuffer, DirectionalShadowMap, PointShadowCubes }`. One header each, so readers depend on the data, not on the stage that makes it. |
| Port | `Graphics/RenderPorts.h` | What `Setup()` hands back: `RenderInput<T>` (`Get`/`TryGet`), `RenderOutput<T>` (`Publish`), `RenderInOut<T>`. A stage can only touch data it declared. |
| `RenderPipeline` | `Graphics/RenderPipeline.h/.cpp` | Owns the stages; compiles the declarations when stages change, then runs them in order for a view. |
| `RenderView` | `Graphics/RenderView.h` | One point of view: camera, viewport size, target, lighting (`Scene` or `Uniform`), auto-exposure and exposure compensation, pick request, and two blackboards. See [Engine-Graphics-Layer.md](Engine-Graphics-Layer.md#views). |
| `RenderContext` | `Graphics/RenderContext.h` | What a stage gets while rendering one view: the view's inputs (read-only), the `RenderScene` being drawn, `DeltaTime`, `FrameIndex` (the same for every view in a frame), `Persistent()` (this view's state across frames) and `ScenePersistent()` (this scene's state across frames, shared by every view that renders it). |

## How the order is worked out

`CompileRenderPipeline()` (`RenderPipelineCompiler.cpp`) reads every stage's declarations and applies a few rules:

| Declaration | Meaning | Ordering rule |
|---|---|---|
| `Writes<T>()` | Produces a new `T` | Runs **first** for that contract. Exactly one writer per contract. |
| `Modifies<T>()` | Changes an existing `T` in place (e.g. draws into its target) | After the writer, **before** any reader. Several modifiers run in the order they were added. |
| `Reads<T>()` | Needs `T` | After the writer and all modifiers. |
| `ReadsOptional<T>()` | Uses `T` if it exists (`TryGet`) | Same as `Reads`, but no error if nobody writes it. |
| `RunsIn(MainOnly)` | Skip secondary views (camera previews) | Not an ordering rule; a filter applied at render time. |

Stages with no dependency between them keep the order they were added in.

**What it catches** (logged, and the pipeline asserts):
- a contract written by more than one stage;
- a stage that reads or modifies a contract nobody writes;
- a stage that runs in every view depending on a contract whose writer is `MainOnly` (it would be missing in camera previews).

## Views: main and secondary

Every render is for one `RenderView`. There are two kinds:

| | Main view | Secondary view |
|---|---|---|
| What it is | The picture shown to the user | An extra picture, e.g. a `CameraComponent` preview or a `ViewCapture` |
| Scene | The active scene's (`RenderEngine::GetMainScene`) | Any `RenderScene`, passed in |
| Rendered by | `RenderEngine::Render()` once per frame | `RenderEngine::RenderSecondaryView()`, on request |
| `MainOnly` stages | Run | Skipped (`PickRenderStage`, `ImGuiRenderStage`, `FinalRenderTargetStage`) |
| Eye adaptation | Adapts smoothly over time | Exposes instantly |

Because stage objects are shared by every view, anything that must carry over between frames *for one view* (like eye-adaptation history) goes in `context.Persistent()`, not in a stage member. Anything that belongs to *the scene*, whichever view draws it (like the shadow maps), goes in `context.ScenePersistent()`.

Work that should happen once a frame per scene, whichever view draws it first, compares `context.FrameIndex` with the frame it last did it in (the probe stages do). `RenderEngine` advances the index after the main view renders, so secondary views rendered earlier in the frame share it.

## The stages

| Stage | Declares | Views | In one line |
|---|---|---|---|
| `ShadowRenderStage` | writes `SceneLightingData` | all | Uploads the scene's lights, and renders their shadow maps when the scene or a light has changed. |
| `DepthPrepassStage` | writes `SceneDepthData` | all | Draws the depth of everything that isn't blended, before any shading. |
| `AmbientOcclusionStage` | reads `SceneDepthData`, writes `AmbientOcclusionData` | all | GTAO from that depth, in three compute passes. |
| `IrradianceProbeStage` | reads `SceneLightingData`, writes `SceneIrradianceData` | all | Captures and relights the scene's grid of irradiance probes, within budgets. |
| `ReflectionProbeStage` | reads `SceneLightingData`, `SceneIrradianceData`, writes `SceneReflectionData` | all | Captures, relights and prefilters the scene's reflection probes. |
| `ForwardRenderStage` | reads `SceneLightingData`, `SceneDepthData`, `AmbientOcclusionData`, `SceneIrradianceData`, `SceneReflectionData`, writes `SceneColourData` | all | Draws every visible object, lit, into an HDR target, over the prepass's depth. |
| `PostProcessRenderStage` | reads `SceneColourData`, `AmbientOcclusionData`, writes `DisplayColourData` | all | Eye adaptation, ACES tonemapping, sRGB encode to 8-bit, FXAA; or the ambient occlusion alone. |
| `PickRenderStage` | nothing | main | On a click or drop, finds the object under one pixel and the depth there. |
| `DebugDrawRenderStage` | modifies `DisplayColourData` | all | Draws debug lines on top, after tonemapping. Not in Retail builds. |
| `ImGuiRenderStage` | Editor: reads `DisplayColourData`; Game: modifies it | main | Draws ImGui. Added by the app, not the engine. |
| `FinalRenderTargetStage` | reads `DisplayColourData` | main | Game only: copies the finished image to the back buffer. |

The engine adds Shadow, DepthPrepass, AmbientOcclusion, IrradianceProbe, ReflectionProbe, Forward, PostProcess, Pick and (outside Retail) DebugDraw in `RenderEngine::Initialize()`. The Editor adds `ImGuiRenderStage(withEditor = true)`; the Game adds `ImGuiRenderStage` and `FinalRenderTargetStage`.

### `ShadowRenderStage`

Turns the scene's lights into what shading reads. Everything it makes is kept **per scene**, in the scene's persistent state (`RenderScene`), so a thumbnail's scene and the editor's each have their own.

- **The light buffer** (`LightBufferLayout`, `b2`): every light, up to `k_maxLights` (16), with its type, position, direction, range, colour, strength and `ShadowIndex` (which of its type's maps it casts into; −1 for none). Uploaded **every time it runs**, since colours and strengths change without changing the revision. Lights past 16 are left out.
- **Which lights cast shadows:** the first directional light, and the first `k_maxShadowedPointLights` (8) point lights, in scene order. One helper (`ShadowAssigner`) decides it for both the light buffer and the shadow rendering, so the indices the shaders read are the maps that were rendered. Point lights past the 8th still light the scene; a warning is logged once.
- **Each type has its own kind of map:**

| Type | Map | Fitted to |
|---|---|---|
| Directional | One 4096² 2D depth map | The bounds of every render object as seen from the light. It doesn't follow the camera, so it never shimmers as the camera moves. |
| Point | One cube of a depth cube array (8 cubes, 512² per face, about 48 MB) | Six 90° views from the light (`CubeMap::CreateFaceView`), from 0.05 m out to its range, one slice each (`Renderer::SetDepthTarget(cubes, cube × 6 + face)`) |

- **`ShadowBuffer`** (`b3`) says how to read them: the sun's view-projection, texel size and normal offset, and per point light the two numbers that turn a distance along a cube face into the depth stored there (`DepthAtInfinity + DepthDistanceScale / distance`, taken from the faces' projection matrix, so the shader uses the same convention that drew them) and its normal offset per metre.
- **The layouts** of both buffers (`LightLayout`, `LightBufferLayout`, `PointShadowLayout`, `ShadowBufferLayout`) and the two limits are in the `GraphicsLayouts` namespace (`Graphics/GraphicsLayouts.h`), not in the stage, since lighting isn't this stage's alone.
- **One way to draw casters:** every view, the sun's and each cube face, goes through `DrawCasters(scene, viewProjection)`, which skips objects and draws outside that view (`Frustum`), e.g. everything behind a cube face.
- **Cached per map:** nothing is checked until the scene's revision (`RenderScene::GetRevision`) changes. Then each map is rendered again only if what it shows changed. Each one remembers the light it was rendered for (its live ID) and the revision it was rendered at (`CachedShadowMap`):

| Map | Rendered again when |
|---|---|
| A point light's cube | Its slot now has another light, the light's `GetShadowRevision()` is newer (it moved, changed range or type), or a render object changed within position ± range, the box its six faces see (`RenderScene::HaveObjectsChangedWithin`). Only that cube's six faces are cleared (`Renderer::ClearDepthStencilSlice`) and drawn. |
| The sun's map | Its light changed in the same way, or any render object changed (`GetObjectsRevision()`), since the map is fitted around them all. |

  So moving one point light renders only its cube, and moving an object renders the sun's map plus the cubes of the point lights near it. The `ShadowBuffer` is kept in the scene's state, each map updates its own part, and it's uploaded again only when a map was rendered. Slots go to lights in scene order, so removing or retyping a light renders the cubes whose slot moved to another light. A slot no light has any more keeps what it held: if its light gets it back unchanged, it's reused.
- A view with `Uniform` lighting ignores the lights, so it doesn't render the maps for it (mesh thumbnails never render one).
- Casters draw with `ERasterizerState::ShadowCaster` (no culling, so thin geometry still casts, plus slope-scaled depth bias), which suits the cube faces' perspective too. `Pixel_ShadowCaster.hlsl` clips cutout materials (leaves, chains), so they cast cut-out shadows.

### `DepthPrepassStage`

Draws the depth of everything that isn't blended before any shading, so ambient occlusion can read it before anything is lit, and so the forward pass shades each pixel once.

- Clears the view's depth and draws opaque and cutout draws with `Vertex_NewRenderer`, the forward pass's vertex shader, so the depths match bit for bit. Opaque draws run **no pixel shader** (`Renderer::UnsetPixelShader`); cutouts run `Pixel_DepthPrepass.hlsl`, which only clips. Blended draws are left out: they don't write depth.
- The same object and submesh culling as the forward pass. Publishes `SceneDepthData { Depth }`.
- In Sponza (Release, uncapped) it made frames 27% faster, 0.84 to 0.61 ms, since hidden pixels are no longer lit; the extra draws cost the render thread about 0.004 ms.

### `AmbientOcclusionStage`

Ground truth ambient occlusion (GTAO), ported from Intel's XeGTAO, from the prepass's depth: how much creases and corners hide each pixel from the light around it. Three compute passes (`Common/GTAO.hlsli` holds what they share):

```mermaid
%%{init: {'flowchart': {'subGraphTitleMargin': {'top': 6, 'bottom': 10}}, 'themeCSS': '.labelBkg, .edgeLabel { background-color: var(--background-primary) !important; } .messageText, .labelText, .loopText { paint-order: stroke; stroke: var(--background-primary); stroke-width: 5px; stroke-linejoin: round; }'}}%%
flowchart TD
    D["View's depth<br/>(SceneDepthData)"] --> P["Compute_GTAOPrefilterDepths<br/>view-space distance + 4 mips"]
    P --> M["Compute_GTAOMain<br/>horizons in a few directions<br/>per pixel, cosine-weighted"]
    M --> E["Visibility + depth edges<br/>(R8_UNORM)"]
    E --> N["Compute_GTAODenoise<br/>edge-aware 3x3, once or twice"]
    N --> V[/"AmbientOcclusionData<br/>visibility, 1 = open"/]
```

- **Levels** (`EAmbientOcclusion`, `Graphics/AmbientOcclusion.h`): Off, Low, Medium, High, XeGTAO's presets (directions x steps per side 1 x 2, 2 x 2, 3 x 3; Low denoises once, the others twice). `RenderView::AmbientOcclusion`, Engine.ini `[Rendering] AmbientOcclusion`, and the Scene View settings.
- **Differences from XeGTAO:** every texture is read with integer `Load`s at a pixel clamped to the viewport, not sampled by UV, since a view can be the top-left part of a bigger target; normals are made from depth; no bent normals; the noise is fixed per pixel (there's no TAA to average it over frames).
- It needs the view's projection on its own (`RenderCamera::Projection`), to turn depth back into positions. Views with AO Off, `Uniform` lighting or no perspective projection get a **1x1 white texture** instead, and readers clamp the pixel to the texture's size, so they never need to check.
- The working textures (depth mips, visibility, edges) are kept per view and remade when its target changes size.
- Costs 0.29 ms a frame on Medium and 0.44 ms on High in Sponza (Release, uncapped, a 2050x1280 view).

### Irradiance and reflection probes

Both kinds of probe capture **what surfaces are around them** rarely, and **relight** those captures often, so moving a light costs a compute pass, not draws.

```mermaid
%%{init: {'flowchart': {'subGraphTitleMargin': {'top': 6, 'bottom': 10}}, 'themeCSS': '.labelBkg, .edgeLabel { background-color: var(--background-primary) !important; } .messageText, .labelText, .loopText { paint-order: stroke; stroke: var(--background-primary); stroke-width: 5px; stroke-linejoin: round; }'}}%%
flowchart TD
    subgraph Capture["Capture (draws): when the grid is fitted, or objects change nearby"]
        direction TB
        C1["ProbeCapture: six faces<br/>per probe, Pixel_ProbeCapture<br/>packs albedo, normal,<br/>distance, back-face bit"]
        C2["Capture atlas<br/>(RG32_UINT)"]
        C1 --> C2
    end
    subgraph Relight["Relight (compute): every frame, within a budget"]
        direction TB
        R1["Lights + shadow maps +<br/>last frame's probes<br/>(Common/ProbeLighting.hlsli)"]
        R2["Irradiance: L1<br/>spherical harmonics<br/>Reflection: radiance<br/>cube, prefiltered mips"]
        R1 --> R2
    end
    C2 --> R1
    R2 --> F["ForwardRenderStage:<br/>ambient diffuse<br/>and reflections"]
```

`ProbeCapture` (`Graphics/ProbeCapture.h/.cpp`) draws one probe's six faces side by side into the bound target, with the forward pass's vertex shader, no culling (`ERasterizerState::NoCull`, so back faces show), and leaves out blended draws and draws smaller than a texel. Each texel stores the nearest surface (`ProbeSurface`, `Common/Probes.hlsli`): sRGB albedo, an octahedral normal, its distance as a float16, and whether the probe sees its back; 0 is a miss, the sky. The faces are plain look-at views in one of two layouts: the irradiance probes' own (`ProbeCapture::GetFaceBasis`), or `CubeMap`'s for reflection probes, so their texels are a cube's.

### `IrradianceProbeStage`

A grid of probes over the scene, whose light the forward pass uses for ambient diffuse.

- **The grid** (`IrradianceProbeGrid::Fit`): over the bounds of every render object, padded by a spacing all round. The spacing scales with the scene's size so a tabletop gets as many probes as an atrium: it starts at cbrt(volume / target) and is corrected by the count it gives, within 0.1 to 4 m unless that would pass 8192 probes. Fitted again when the scene's bounds move by more than a spacing. Sponza: 4416 probes, 1.9 m apart.
- **Each probe** captures 6 faces of 8x8 texels (384 samples) into the scene's capture atlas. Right after, `Compute_ProbeVisibility` builds its **visibility map**: an 8x8 octahedral map (`Utils/Math/Octahedral.h`) of the mean and mean squared distance it sees, by direction, with a mirrored 1-texel border so filtering wraps. A probe that sees more than 25% back faces is inside geometry: its map is all 0, which marks it **inactive**.
- **Relighting** (`Compute_ProbeRelight`, a thread group per probe): each captured surface is lit by the scene's lights past their shadows and by last frame's probes (so light bounces once more each frame), the sky lights what sees nothing, and back faces give nothing. The 384 values reduce to order-1 spherical harmonics (`Utils/Math/SphericalHarmonics.h`'s conventions), three texture arrays (R, G, B), a probe per texel and a slice per grid layer. Last frame's are a `CopyTexture` of them.
- **Scheduling**, once a frame per scene, by whichever view draws it first, for views with `RenderView::GlobalIllumination` and `Scene` lighting:
  - The textures are cleared when the grid is fitted, so probes not captured yet read as inactive and probes not relit yet as black; the light fills in from the camera outwards.
  - Captures come off a queue nearest the camera first, until `ProbeCaptureDrawBudget` draws (Sponza's first capture takes 76 frames).
  - Each frame relights its captures, then the probes within 2 spacings of the camera, then the rest in turn, up to `ProbeRelightBudget` (512).
  - When objects change (`RenderScene::GetObjectsRevision`), the probes within 5 spacings of them (`HaveObjectsChangedWithin`) are queued to be captured again; moving Sponza's tank 2 m queues 1144 of 4416.
  - The budgets and the probe count are `GlobalIlluminationSettings` (`Graphics/GlobalIllumination.h`), from Engine.ini's `[Rendering]`, owned by `RenderEngine`.
- **Publishes** `SceneIrradianceData`: the volume's layout (`GraphicsLayouts::IrradianceVolumeLayout`, `b5`), the SH textures (`t8` to `t10`) and the visibility atlas (`t11`); placeholders, and a probe count of 0, for views without global illumination.
- Costs 0.25 ms a frame in Sponza (Release, uncapped).

### `ReflectionProbeStage`

The scene's `ReflectionProbeObject`s (placed with `ReflectionProbeComponent`), which glossy surfaces in their boxes reflect. Up to `GraphicsLayouts::k_maxReflectionProbes` (16), the first the scene made, a slot each, with the same once-a-frame rule as the irradiance probes. A scene with no probes allocates nothing. Each frame:

1. **One capture** at most, at 128x128 a face in `CubeMap`'s layout: a probe that's new, moved or resized, or one an object changed in the box of.
2. **Up to two relit** (`Compute_ReflectionRelight`, the same surface lighting as the irradiance probes, which they're also lit by) into a radiance cube array, which then gets its mips (`GenerateMips`).
3. **Prefiltered** (`Compute_ReflectionPrefilter`) into 6 mips: mip 0 is the radiance, each further one the Phong lobe of the shininess the forward pass maps that mip to, importance-sampled 64 times from the radiance mip that matches each sample's share of the lobe.

It publishes `SceneReflectionData`: the lit probes' boxes (`GraphicsLayouts::ReflectionProbeBufferLayout`, `b7`) and the prefiltered cube array (`t12`).

### `ForwardRenderStage`

- Draws into the view's HDR scene target (`RGBA16_FLOAT`, alpha 0 marks the background) over the prepass's depth, which it doesn't clear: opaque and cutout draws test `EDepthState::ReadOnlyLessEqual` (pass on equal, no writes), blended ones `ReadOnly`.
- **Frustum culling** at two levels: whole objects first, then each submesh, using the world-space bounds every `RenderObject` keeps (`Frustum`, in `Utils/Math/Frustum.h`).
- Per-view constants are bound once; per object only the world matrix changes. Pixel shaders are only re-bound when the next submesh uses a different one. Every texture the next frame writes is unbound after the pass.

| Register | What | From |
|---|---|---|
| `b2`, `b3` | `LightBuffer`, `ShadowBuffer` | `SceneLightingData` |
| `b4` | `ViewLightingBuffer`: uniform lighting, the indirect-light debug view | the view |
| `b5`, `t8`-`t11` | the irradiance probes: their layout, SH textures, visibility atlas | `SceneIrradianceData` |
| `b7`, `t12` | the reflection probes: their boxes, prefiltered cube array | `SceneReflectionData` |
| `t5`, `t6` | the sun's shadow map, the point light cubes | `SceneLightingData` |
| `t7` | ambient occlusion (the white texture for blended draws, which aren't in the depth it came from) | `AmbientOcclusionData` |

- Shading lives in `Assets/Shaders/Common/Lighting.hlsli`, and the lights, shadow maps and sky in `Common/LightResources.hlsli` (so the probes' compute shaders can light with them too). For each light, its type picks the direction to it and its falloff (`LightAttenuation`) and its shadow lookup (`LightShadow`), while the diffuse and specular are shared (`DirectLighting`). Plus normal and height maps, and opacity masks.
- **Ambient light** (`AmbientLighting`), all of it darkened by ambient occlusion and none of the lights' direct light:
  - Diffuse from the irradiance probes (`SampleProbeIrradiance`, `Common/IrradianceVolume.hlsli`): the 8 around the surface, weighted by distance, by which side of the surface they're on, and by DDGI's Chebyshev test against their visibility maps, skipping inactive ones. The hemisphere where none can be read. Occluded through Jimenez's multi-bounce fit, so bright surfaces don't go grey in creases.
  - Reflections from the reflection probes whose boxes the surface is in (`ReflectionEnvironment`, `Common/ReflectionProbes.hlsli`), faded in over their blend distance and projected onto their boxes, at the mip for the surface's gloss; the hemisphere for the rest. Occluded by Lagarde's specular occlusion. Outside probes, rough reflections are also dimmed (gloss squared), since nothing else occludes the sky there.
- `RenderView::Debug`'s `IndirectDiffuse` makes it output the ambient diffuse alone, as it would light a white surface.
- **Shadow lookups by type:** `SampleDirectionalShadow` is a 4×4 grid of filtered comparisons in the sun's map. `SamplePointShadow` turns the surface's direction from the light into the depth to compare with (its largest component is its distance along the face it falls in), and averages four filtered comparisons slightly around that direction. Cube faces join seamlessly in hardware, so those can cross between faces. Cube sampling flips Z (`ToCubeSampleDirection`, see [CubeMap](../06-Platform-And-Infrastructure/Utils-Library-Reference.md)).
- With `Uniform` lighting, `ComputeLighting` skips the lights and their shadows and lights the surface from an even white environment (`UniformAmbient`), diffuse and reflections alike. Faces are still shaded from 1 facing the camera to `UniformEdgeShade` (0.45) edge-on, as if lit from the camera, since even light alone turns untextured meshes into silhouettes.

### `PostProcessRenderStage`

Turns linear HDR colour into something a monitor can show, then smooths its jagged edges:

```mermaid
%%{init: {'flowchart': {'subGraphTitleMargin': {'top': 6, 'bottom': 10}}, 'themeCSS': '.labelBkg, .edgeLabel { background-color: var(--background-primary) !important; } .messageText, .labelText, .loopText { paint-order: stroke; stroke: var(--background-primary); stroke-width: 5px; stroke-linejoin: round; }'}}%%
flowchart TD
    S["Scene colour<br/>(HDR)"] --> L["Log luminance<br/>256x256, mips down to 1x1"]
    L --> A["Eye adaptation<br/>blend last frame's value<br/>toward the new average"]
    A --> T["Expose x ACES tonemap<br/>x sRGB encode"]
    S --> T
    T --> Q{"view's AntiAliasing<br/>Off?"}
    Q -->|yes, straight in| D["Display colour<br/>(8-bit)"]
    Q -->|no| I["Tonemapped<br/>(8-bit, per view)"]
    I --> F["FXAA"]
    F --> D
```

- **Debug view:** with `RenderView::Debug` at `AmbientOcclusion`, the finished image is overwritten by the ambient occlusion alone, in grey (`Pixel_DebugAmbientOcclusion.hlsl`).
- **Eye adaptation** moves toward the scene's log-average luminance at about 1.5 s to settle, clamped to −3…+4 EV, then `RenderView::ExposureCompensation` is added on top. Its 1×1 history is ping-ponged between two textures kept in the view's persistent blackboard.
- A view with `AutoExposure` off skips measuring and adapting, and the tonemap exposes at a fixed 1 (plus `ExposureCompensation`), so what's drawn sets the brightness. Mesh thumbnails use this.
- The tonemap pass leaves the scene **depth** bound to `DisplayColourData`, so overlays drawn later (debug lines) can still depth-test against the scene. So does FXAA, and debug lines are drawn after it, so they aren't smoothed.

#### Anti-aliasing (FXAA)

Unless `RenderView::AntiAliasing` is `Off`, the tonemap draws into an intermediate 8-bit texture (`AntiAliasState`, kept in the view's persistent blackboard and remade when the view's target changes size), and `Pixel_FXAA.hlsl` smooths it into the display target. It's a port of FXAA 3.11 Quality, run on the tonemapped, sRGB-encoded image so its contrast thresholds mean the same in shadow as in sunlight. For each pixel:

1. **Is it an edge?** If the contrast between the pixel and its four neighbours is under `max(EdgeThresholdMin, brightest × EdgeThreshold)`, it passes through unchanged. Most pixels stop here, so texture detail stays sharp.
2. **Which way does the edge run, and which side is it on?** Second differences over the 3×3 neighbourhood pick horizontal or vertical (they catch one-pixel-thin lines too); the neighbour across the edge with the bigger step picks the side.
3. **How long is the stair-step?** It samples half a pixel across, exactly on the boundary, where bilinear filtering averages the two pixels, and walks both ways along the edge until that average changes. The search steps grow (Medium: 1, 1.5, 2, 4, then an unsampled 12), and an end not found by the last step is assumed to be that far.
4. **How far to move the sample?** `0.5 − distance to the nearer end / step length`: about half a pixel at the ends of a step, none in the middle, and none toward an end on the other side of the step. A pixel that stands out from its whole neighbourhood (detail thinner than a pixel) is moved by up to `Subpixel` instead, if that's more.
5. The pixel's colour is one bilinear sample moved that far across the edge: a blend with its neighbour.

The levels are FXAA 3.11's quality presets (`GetAntiAliasingSettings`, `Graphics/AntiAliasing.h`), uploaded in `PostProcessParams`:

| Level | Preset | EdgeThreshold | EdgeThresholdMin | Subpixel | Search steps (reach) |
|---|---|---|---|---|---|
| Low | 10 | 0.250 | 0.0833 | 0.25 | 1.5, 3, 12 (16.5 px) |
| Medium (default) | 12 | 0.166 | 0.0625 | 0.50 | 1, 1.5, 2, 4, 12 (20.5 px) |
| High | 39 | 0.125 | 0.0312 | 0.75 | 1 ×5, 1.5, 2 ×4, 4, 8 (26.5 px) |

Higher levels treat fainter contrast as an edge, find the ends of long edges more accurately, and soften more sub-pixel sparkle, at the cost of more texture blur and more samples on edge pixels (5 for a pixel that isn't an edge; at most 14, 18 or 32 for one that is). FXAA works on single frames, so it softens shimmer on thin geometry but doesn't remove it.

### `PickRenderStage`

Finds the object at one pixel of the main view, and how far away its surface is, for the Editor's click selection and for placing meshes dropped into the SceneView. It only does work on frames where `RenderView::Pick` holds a `RenderPickRequest { X, Y, Result, DepthResult }`; `RenderEngine::Render()` clears the request afterwards.

```mermaid
%%{init: {'flowchart': {'subGraphTitleMargin': {'top': 6, 'bottom': 10}}, 'themeCSS': '.labelBkg, .edgeLabel { background-color: var(--background-primary) !important; } .messageText, .labelText, .loopText { paint-order: stroke; stroke: var(--background-primary); stroke-width: 5px; stroke-linejoin: round; }'}}%%
sequenceDiagram
    participant Ed as Editor (SceneViewPanel)
    participant View as main RenderView
    participant Pick as PickRenderStage
    participant RT as Render thread
    Ed->>View: Pick = { pixel, ID and depth TexelReadbacks }
    Note over Pick: next frame
    Pick->>Pick: stretch that pixel over a 1x1 R32G32_UINT target<br/>and a 1x1 depth target<br/>(Matrix4x4::CreateSubRectProjection)
    Pick->>Pick: draw nearby objects' pick IDs<br/>(frustum culling skips the rest)
    Pick->>RT: Renderer::ReadTexel the ID, then the depth
    RT->>RT: copy to staging textures, Map, store values
    RT-->>Ed: TexelReadback::IsReady = true
    Ed->>Ed: pick ID - 1 = GameObjectID<br/>depth -> world position
```

- **Cheap by design:** 1×1 targets mean nothing to resize with the window, the one-pixel frustum culls nearly everything, and only 12 bytes come back (the 8-byte ID and the 4-byte depth).
- **64-bit IDs:** pick IDs are U64 because `GameObjectID` is, so the ID target is `R32G32_UINT` and `Pixel_PickID.hlsl` writes a `uint2`, low half then high.
- **Depth:** `DepthResult` is optional. It gets the depth target's texel (D24S8), which `RenderPickRequest::DecodeDepth` turns into 0 at the near plane to 1 at the far plane. The depth target is cleared to 1, so 1 means nothing is drawn there. The Editor's `ScenePicker` unprojects it with the camera it asked with (`ViewProjection::Unproject`) to find where the surface is.
- `Pixel_PickID.hlsl` writes each `RenderObject::GetPickID()`. Objects with ID 0 still draw, so they hide what's behind them. Cutout pixels are clipped as in the shadow pass, so clicks go through them.
- The readback makes the render thread wait for the GPU, but only on a click frame. See [`ReadTexel`](Low-Level-Renderer-Module.md#reading-back-from-the-gpu-readtexel-and-readtexture).

### `DebugDrawRenderStage`

Draws the rendered scene's debug lines, queued with `RenderScene::DrawLine()` / `DrawWireBox()` / `DrawWireSphere()` or `RenderEngine`'s versions for the main scene (bounds boxes, camera frustums, a directional light's direction, point light ranges, the editor grid), on top of the display colour. A view with `DrawsDebug` off (a `ViewCapture`'s) skips it. They're drawn after tonemapping so they keep their exact colours. They depth-test against the scene; lines with a `DebugLine::occludedAlpha` above 0 are then drawn again with the depth test off (`EDepthState::Off`) at that opacity, from a second buffer, so their hidden parts still show, faintly. That also goes over their visible parts, which are already drawn more opaque, so it barely changes those. Lines can be semi-transparent: the stage turns on `EBlendState::AlphaBlend` for its draw and restores `Opaque` after. Each vertex also carries an optional distance fade (`FADECENTRE`, `FADERANGE`), which `Pixel_Line.hlsl` applies per pixel; pixels that fade to nothing are clipped so they don't write depth. The queue is cleared after the main view renders, so secondary views drawn earlier in the frame get them too. Compiled out of Retail builds.

### `ImGuiRenderStage` and `FinalRenderTargetStage`

- **Editor:** ImGui draws the whole UI to the back buffer; the scene view panel shows the display colour target as an `ImGui::Image`, so the stage *reads* `DisplayColourData`.
- **Game:** ImGui draws over the display colour (*modifies* it), then `FinalRenderTargetStage` copies it to the back buffer.

Both are `MainOnly`: `ImGui::Render()` ends ImGui's frame, so it can only run once per frame.

## Writing a new stage

1. Add a data contract in `Graphics/RenderData/` if the stage produces something new.
2. Implement `IRenderStage`. In `Setup()`, declare everything and keep the ports:

    ```cpp
    void MyStage::Setup(RenderStageBuilder& builder)
    {
        m_sceneColour = builder.Reads<SceneColourData>();
        m_output = builder.Writes<MyData>();
        builder.RunsIn(ERenderViewFilter::MainOnly); // only if it must skip previews
    }

    void MyStage::Render(RenderContext& context)
    {
        const SceneColourData& scene = m_sceneColour.Get(context);
        // ... draw with Rendering::Renderer ...
        m_output.Publish(context, MyData{ /* handles */ });
    }
    ```

3. Load its shaders in `Initialize()` into `TypedResourceHandle` members, by the file's stem in `Assets/Shaders`: `m_pixelShader = LoadPixelShader("Pixel_Tonemap");` (and `LoadVertexShader(name, inputLayout)`). Draw with `m_pixelShader->GetPlatformObject()`. The members keep the shaders loaded until the stage is destroyed.
4. Register it: `RZE().GetRenderEngine().AddRenderStage<MyStage>()`. Where it lands in the order follows from the declarations; check the logged `Render pipeline:` line.

Keep per-view, across-frame state in `context.Persistent()`, and add new per-frame inputs to `RenderView`/`RenderContext` rather than reading globals.

## Key files

- `Engine\Src\Graphics\RenderStage.h`, `RenderStageBuilder.h`, `RenderPorts.h`, `RenderBlackboard.h`, `RenderContext.h`
- `Engine\Src\Graphics\RenderPipeline.h/.cpp`, `RenderPipelineCompiler.h/.cpp`
- `Engine\Src\Graphics\RenderData\` — `SceneLightingData.h`, `SceneDepthData.h`, `AmbientOcclusionData.h`, `SceneIrradianceData.h`, `SceneReflectionData.h`, `SceneColourData.h`, `DisplayColourData.h`
- `Engine\Src\Graphics\AmbientOcclusion.h/.cpp` — `EAmbientOcclusion` levels and their GTAO settings
- `Engine\Src\Graphics\ProbeCapture.h/.cpp`, `IrradianceProbeGrid.h/.cpp`, `GlobalIllumination.h` — probe capture, the irradiance grid, the probe budgets
- `Engine\Src\Graphics\ReflectionProbeObject.h/.cpp` — a reflection probe in a `RenderScene`
- `Engine\Src\Graphics\RenderStages\` — one `.h/.cpp` per stage
- `Engine\Src\Graphics\GraphicsLayouts.h` — the CPU side of shader buffers the stages share, in `namespace GraphicsLayouts`: the light and shadow layouts and their limits, `ViewLightingLayout`, an object's matrices (`ObjectBufferLayout`, the vertex shaders' `ObjectBuffer`), a material's parameters (`MaterialBufferLayout`, the pixel shaders' `MaterialBuffer`), and the probes' (`IrradianceVolumeLayout`, `ReflectionProbeBufferLayout`, and `ProbeListLayout`, which both probe stages' compute passes take). A layout only one stage's shaders use stays in that stage (`PickRenderStage::PickBufferLayout`, `PostProcessRenderStage::ParamsLayout`, `AmbientOcclusionStage::ParamsLayout`); put new ones by the same rule.
- `Engine\Src\Graphics\AntiAliasing.h/.cpp` — `EAntiAliasing` levels and their FXAA settings
- `Assets\Shaders\` — `Pixel_ShadowCaster`, `Pixel_DepthPrepass`, `Pixel_NewRenderer*`, `Pixel_ProbeCapture`, `Pixel_LuminanceLog`, `Pixel_ExposureAdapt`, `Pixel_Tonemap`, `Pixel_FXAA`, `Pixel_DebugAmbientOcclusion`, `Pixel_PickID`, `Pixel_Line`; compute: `Compute_GTAO*`, `Compute_ProbeVisibility`, `Compute_ProbeRelight`, `Compute_ReflectionRelight`, `Compute_ReflectionPrefilter`
- `Assets\Shaders\Common\` — `Lighting.hlsli` (surface shading), `LightResources.hlsli` (lights, shadows, sky), `PixelResources.hlsli` (the forward pass's bindings), `GTAO.hlsli`, `Probes.hlsli` (capture packing, face layouts, probe lists), `ProbeLighting.hlsli`, `IrradianceVolume.hlsli`, `ReflectionProbes.hlsli`
