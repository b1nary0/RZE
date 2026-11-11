# Built-In Components Reference

All built-in components live under `Engine\Src\Game\World\GameObjectComponents\` and are registered by `RZE_Application::Initialize(window)`, before the app's own `OnInitialize()`. Each inherits `GameObjectComponent<Self>` (see [Component-System.md](Component-System.md)).

## `TransformComponent`

Every `GameObject` created via `GameScene::CreateGameObject()` gets one automatically. Holds position/rotation/scale and maintains its own parent/child hierarchy (`AttachTo` / `DetachFromParent`), used to compute world transforms relative to a parent transform.
- **The world matrix is worked out when it's read.** `GetWorldMatrix()` recomputes it from the parent's world matrix and `GetLocalMatrix()` only if it's out of date (`MarkDirty()`, which also marks every child), then keeps it. There's no per-frame `Update()`, so the matrix is right straight after a move, and in a scene that's never updated, such as a [`HeadlessScene`](Scene-And-GameObject-Model.md#headlessscene-a-scene-rendered-off-screen).
- **Bounds:** moving a child tells its parent's `GameObject` that its hierarchy bounds have changed (`MarkHierarchyBoundsChanged`). A root moving changes nobody's extents, only where they're placed. See [Scene-And-GameObject-Model.md](Scene-And-GameObject-Model.md#bounds).
- **Axes:** `GetForward()` and `GetUp()` are `WorldAxes::Forward()` (−Z) and `Up()` turned by its own rotation, normalized. Parents' rotations aren't included. `CameraComponent` aims along them, and a directional `LightComponent` shines along `−GetUp()`.

## `RenderComponent`

Bridges a GameObject to the rendering system. Holds a `StaticMeshInstance` (its mesh as this object draws it, with its material slots; see [Materials-Meshes-Shaders.md](../04-Rendering/Materials-Meshes-Shaders.md#shared-meshes-per-object-instances)) and a `RenderObjectPtr` into **its own scene's** `RenderScene`, which it keeps from `Initialize()` (`GetOwner()->GetScene().GetRenderScene()`).
- **Render object:** created in `OnAddToScene()`, seeded with the sibling `TransformComponent`'s current world matrix. `Update()` re-pushes the transform every frame, so moving the `TransformComponent` moves the rendered mesh; the render object ignores unchanged matrices. The render object also gets a **pick ID** (`GameObjectID + 1`), so the Editor can click-select it.
- **Changing the mesh:** `SetMeshInstance(instance, overrides)` is the one way the mesh changes. The render object keeps its identity and swaps what it draws, and null removes the mesh. `EMaterialOverrides::CarryOver` (the default) moves the previous instance's material overrides onto the new one by slot name; `UseNew` keeps a prepared instance's own. `LoadMesh(path)` loads synchronously (a cache hit when preloaded), then calls it. For runtime swaps without a stall, preload first ([Materials-Meshes-Shaders.md](../04-Rendering/Materials-Meshes-Shaders.md#swapping-a-mesh-at-runtime)).
- **Mobility:** `SetMobility(ERenderMobility)`, **Static** (the default) for what stays put or Movable for what moves while the scene runs; its render object follows. Static objects are captured into the scene's probes and fit the probe grid and the sun's shadow map. Movable ones are lit by the probes but not captured into them, so moving them costs the probes nothing ([`RenderObject`](../04-Rendering/Engine-Graphics-Layer.md#renderobject)).
- **Saving:** the mesh path, its mobility by name (`"Mobility": "Static" | "Movable"`; missing or unknown loads Static), and the instance's material overrides by slot name under `"MaterialOverrides"` (only what's overridden).
- **Loading:** a static `GatherResourceDependencies()` tells `SceneLoadOperation` which mesh a serialised `RenderComponent` needs, and which materials and textures its overrides swap in. They're loaded asynchronously before the object is created ([Asset-Import-Runtime.md](../05-AssetPipeline/Asset-Import-Runtime.md)).
- **Inspector** (described properties):
  - **Mesh:** an asset path. Its `...` button opens the [file browser](../07-Editor/Thumbnails-And-File-Browser.md) on the mesh thumbnails (or a mesh can be dropped on it from the Content Browser), its setter `LoadMesh(path)` loads it through the resource system, and it can be cleared. It's `.Preload<StaticMeshResource>()`, so the editor loads a new mesh in the background first and sets it once it's in ([Asset fields](../07-Editor/Object-Inspector-And-Properties.md#asset-fields)); a material slot's Material and textures are preloaded the same way.
  - **Size:** in metres (mesh bounds × transform scale).
  - **Mobility:** a drop-down, Static or Movable.
  - **Debug Draw:** Mesh Bounds (yellow) and Sub-Mesh Bounds (blue).
  - **Materials:** an editable list of the mesh's material slots. Each slot's material asset, tint, blend mode, opacity, shininess, textures and mask flags can be overridden for this object. See [Object-Inspector-And-Properties.md](../07-Editor/Object-Inspector-And-Properties.md#whats-described-today).
- `GetWorldBounds()` and `GetRenderObject()` expose the world-space bounds for framing, marquee selection and mesh thumbnails. `GetLocalBounds()` (the mesh's box in the object's own space) feeds its `GameObject`'s hierarchy bounds.

## `CameraComponent`

A scene camera: FOV (degrees), near/far clip (0.1 m / 1000 m in the shipped scenes) and exposure compensation (EV).
- **Orientation comes from the owner's `TransformComponent` rotation** (`TransformComponent::GetForward/GetUp`): −Z is forward, +Y is up. The Editor's rotate gizmo and the Rotation fields aim it; there's no separate look-at state.
- When it's the scene's **main camera** ("Main Camera" in the inspector, saved as `"MainCamera"`), `Update()` writes its exposure compensation, and its `ClipSpace` (projection × view, 0..1 depth) and position, into `RZE().GetRenderEngine().GetMainView()`. The matrices are skipped while `GameScene::DoesMainCameraDriveView()` is false, as in the Editor, whose own camera drives the view. A scene can have any number of cameras; only one is main. See [Scene-And-GameObject-Model.md](Scene-And-GameObject-Model.md#the-main-camera).
- `SetMainCamera(true)` makes it the main camera through `GameScene::SetMainCamera`, which clears the flag on the previous one; `SetMainCamera(false)` clears the role if it has it. The inspector property is `Exclusive`, so it can only be turned on (see [Object-Inspector-And-Properties.md](../07-Editor/Object-Inspector-And-Properties.md#metadata)). A pasted camera is never main.
- Only the **active scene's** main camera drives the main view; a camera in another scene (for example a thumbnail's `HeadlessScene`) never does.
- `CreateRenderCamera(viewportSize)` builds a `RenderCamera` from the camera at that size's aspect ratio without touching the camera itself. Scene thumbnails use it at 512×512.
- Owns a small render target and a secondary `RenderView`. `RenderPreview()` renders the **"Camera View" preview** into it with its own local projection, leaving the camera untouched ([Engine-Graphics-Layer.md](../04-Rendering/Engine-Graphics-Layer.md#views)). The inspector shows it through a `Preview` property, which renders it each frame it's shown.
- Its inspector fields (Projection: Field Of View, Near Cull, Far Cull; Exposure: Compensation) go through `SetFOV`, `SetNearCull`, `SetFarCull` and `SetExposureCompensation`. `SetFOV` and `SetNearCull` assert the value is positive, so the inspector's ranges keep them above 0. An active camera rebuilds its matrices every `Update()`, and the preview builds its own each time it renders, so changes show straight away.
- In the Editor, every scene camera shows a movie-camera icon and its frustum in the scene view.

## `LightComponent`

A light of any type (`ELightType`), picked from a drop-down: **Point** (the default, since most lights you add are) or **Directional**. Wraps a `LightObjectPtr` into its own scene's `RenderScene`, created on `OnAddToScene()` and destroyed on `OnRemoveFromScene()`. See [`LightObject`](../04-Rendering/Engine-Graphics-Layer.md#lightobject) for what each type does.
- **`Update()` pushes everything every frame**: type, position (the transform's), direction (`−TransformComponent::GetUp()`, so a zero rotation is a straight-down sun), range, colour and strength. The light object's setters ignore unchanged values and values its type doesn't read; a change bumps the scene's revision and the light's lighting revision, and its shadow revision only if it moves its shadows ([`LightObject`](../04-Rendering/Engine-Graphics-Layer.md#lightobject)). Changing the type changes the light in place.
- **Flicker** (0 to 1): how much the strength flickers, like a flame. 0 is steady; 1 dips all the way to dark. While it's above 0, the strength the light object gets each frame is `CalculateFlickeringStrength()`, the set strength dipped by two sine waves (5.3 and 11.7 Hz, repeating every 10 s), so a flickering point light changes the light around it every frame, and the probes there relight. The strength shown and saved is the set one.
- **Inspector:** Type (drop-down), Colour (colour picker), Strength (0–100), Range (0.1–100 m), which is greyed out unless the light is a point light (`EnabledIf`), and Flicker (0–1).
- **Saving:** `"LightComponent": { "Type": "Point" | "Directional", "Range", "LightStrength", "Flicker", "LightColour": [r, g, b] }`. A missing Flicker loads as 0. A missing or unknown type loads as a point light, like a new one. Scenes saved before lights had types used `"DirectionalLightComponent"`; every scene in the repo has been rewritten, and that name no longer loads.
- **Editor:** a sun icon for a directional light, with a red line showing which way it shines; a bulb icon for a point light, with its range drawn as a pale yellow wire sphere and, inside it, an orange one out to where it stops outshining the ambient light (grows with strength). See [Editor-Panels.md](../07-Editor/Editor-Panels.md#sceneview-sceneviewpanel).

## `ReflectionProbeComponent`

Places a reflection probe ([`ReflectionProbeObject`](../04-Rendering/Engine-Graphics-Layer.md#reflectionprobeobject)) at its object's position, which captures the scene around it for glossy surfaces in its box to reflect. The box is centred on the object and aligned to the world's axes, so it doesn't turn with the object. Wraps a `ReflectionProbeObjectPtr` into its own scene's `RenderScene`, created on `OnAddToScene()` and destroyed on `OnRemoveFromScene()`.
- **`Update()`** pushes the position, the box's half size and the blend distance every frame; moving or resizing the probe makes `ReflectionProbeStage` capture it again once it has stayed still for a few frames ([Render-Stages.md](../04-Rendering/Render-Stages.md#reflectionprobestage)).
- **Inspector:** Size (metres along each world axis, 8 × 4 × 8 by default; fit it to the room, since reflections are projected onto its sides) and Blend Distance (0–10 m, how far inside the box's sides its reflection fades in; 0.5 by default).
- **Saving:** `"ReflectionProbeComponent": { "Size": [x, y, z], "BlendDistance" }`.
- A scene uses the first 16 it made (`GraphicsLayouts::k_maxReflectionProbes`). The Editor shows each as a shiny-ball icon with its box.

## `OscillatorComponent` (development builds only)

A development aid for stress and test scenes (`RenderPerfTest.scene`'s moving light and Nanosuit): it moves its object back and forth along an axis. It exists only with `RZE_DEVEL` (Debug and Development), so Retail doesn't register it, and a scene using it loads there without it (`[name] has a component this build doesn't have, [OscillatorComponent]; it was skipped.`).
- **The motion:** a sine wave `Amplitude` metres either side of `Origin` along `Axis` (any length; only its direction counts), one cycle every `Period` seconds, starting `Phase` of a cycle in (0.25 starts at the top of the swing). The position comes from the time alone (`GetOffsetAt(seconds)`), kept within one period, so it never drifts. `Update()` calls `Advance(RZE().GetDeltaTime())`; a zero amplitude, axis or period doesn't move it.
- **While it's on, it owns the position:** a gizmo drag or a Transform edit is undone the next frame. Move an oscillating object by changing its Origin, or turn it off.
- **Enabled:** off puts the object back at its Origin; on makes wherever the object is then its Origin. A component added to an object takes the object's position as its Origin (`Initialize()`); a loaded one reads its own.
- **Inspector:** Enabled, Axis, Amplitude (0–100 m), Period (0.05–60 s), Phase (0–1 cycles), and Origin, greyed out while it's off.
- **Saving:** `"OscillatorComponent": { "Enabled", "Axis": [x, y, z], "Amplitude", "Period", "Phase", "Origin": [x, y, z] }`; whatever's missing keeps a new component's value.

## `PlayerControllerComponent`

Gameplay-facing first-person-style controller: movement speed, mouse yaw/pitch/roll, up/forward vectors. Attached at runtime (not scene-serialized) by `GameApp::Start()` onto the loaded scene's main camera. Binds axes/actions through `RZE().GetInputHandler()`.

## `GifChatComponent`

A novelty/demo component (animated GIF display/overlay). Appears to be experimental/fun content rather than core engine functionality — flagged here so it isn't mistaken for a load-bearing system when reading component lists.

## `EditorCameraComponent` (Editor-only — not registered in Game builds)

Documented in [07-Editor/Editor-Camera-And-Build-Launch.md](../07-Editor/Editor-Camera-And-Build-Launch.md). Free-fly viewport camera; also the concrete example of the component-inheritance ID bug described in [Component-System.md](Component-System.md), since it inherits from `CameraComponent`.

## Summary table

| Component | Registered by | Scene-serialized? | Depends on |
|---|---|---|---|
| `TransformComponent` | `RZE_Application::RegisterEngineComponents()` | Yes | — (always present) |
| `RenderComponent` | `RZE_Application::RegisterEngineComponents()` | Yes | `TransformComponent` (sibling), `RenderEngine`, `ResourceHandler` |
| `CameraComponent` | `RZE_Application::RegisterEngineComponents()` | Yes | `TransformComponent` (orientation), `RenderEngine::GetMainView()`, `GameScene` (main camera) |
| `LightComponent` | `RZE_Application::RegisterEngineComponents()` | Yes | `TransformComponent` (position, direction), `RenderScene` light objects |
| `ReflectionProbeComponent` | `RZE_Application::RegisterEngineComponents()` | Yes | `TransformComponent` (position), `RenderScene` reflection probes |
| `OscillatorComponent` | `RZE_Application::RegisterEngineComponents()`, `RZE_DEVEL` only | Yes (skipped when loaded in Retail) | `TransformComponent` (position) |
| `PlayerControllerComponent` | not registered | No — attached at runtime | `InputHandler`, `TransformComponent` |
| `GifChatComponent` | `RZE_Application::RegisterEngineComponents()` | Yes | — |
| `EditorCameraComponent` | `Editor::EditorApp`'s constructor | No — excluded via `SetIncludeInSave(false)` | `CameraComponent` (inheritance — buggy ID), `RenderEngine`, `InputHandler` |

## Key files

- `Engine\Src\Game\World\GameObjectComponents\TransformComponent.h/.cpp`
- `Engine\Src\Game\World\GameObjectComponents\RenderComponent.h/.cpp`
- `Engine\Src\Game\World\GameObjectComponents\CameraComponent.h/.cpp`
- `Engine\Src\Game\World\GameObjectComponents\LightComponent.h/.cpp`
- `Engine\Src\Game\World\GameObjectComponents\ReflectionProbeComponent.h/.cpp`
- `Engine\Src\Game\World\GameObjectComponents\OscillatorComponent.h/.cpp`
- `Engine\Src\Game\World\GameObjectComponents\PlayerControllerComponent.h/.cpp`
- `Engine\Src\Game\World\GameObjectComponents\GifChatComponent.h/.cpp`
- `Editor\Src\Game\World\GameObjectComponents\EditorCameraComponent.h/.cpp`
