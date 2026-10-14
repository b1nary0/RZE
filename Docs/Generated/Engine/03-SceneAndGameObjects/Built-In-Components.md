# Built-In Components Reference

All built-in components live under `Engine\Src\Game\World\GameObjectComponents\` and are registered by the base `RZE_Application::Initialize()`. Each inherits `GameObjectComponent<Self>` (see [Component-System.md](Component-System.md)).

## `TransformComponent`

Every `GameObject` created via `GameScene::CreateGameObject()` gets one automatically. Holds position/rotation/scale and maintains its own parent/child hierarchy (`AttachTo` / `DetachFromParent`), used to compute world transforms relative to a parent transform.
- **The world matrix is worked out when it's read.** `GetWorldMatrix()` recomputes it from the parent's world matrix and `GetLocalMatrix()` only if it's out of date (`MarkDirty()`, which also marks every child), then keeps it. There's no per-frame `Update()`, so the matrix is right straight after a move, and in a scene that's never updated, such as a [`HeadlessScene`](Scene-And-GameObject-Model.md#headlessscene-a-scene-rendered-off-screen).
- **Bounds:** moving a child tells its parent's `GameObject` that its hierarchy bounds have changed (`MarkHierarchyBoundsChanged`). A root moving changes nobody's extents, only where they're placed. See [Scene-And-GameObject-Model.md](Scene-And-GameObject-Model.md#bounds).

## `RenderComponent`

Bridges a GameObject to the rendering system. Holds a `ResourceHandle` to a `StaticMeshResource` and a `RenderObjectPtr` into **its own scene's** `RenderScene`, which it keeps from `Initialize()` (`GetOwner()->GetScene().GetRenderScene()`). On `OnAddToScene()`, creates the render object there, seeded with the sibling `TransformComponent`'s current world matrix. `Update()` re-pushes the transform every frame (so moving the `TransformComponent` moves the rendered mesh; the render object ignores unchanged matrices). When it creates the render object it also gives it a **pick ID** (`GameObjectID + 1`), so the Editor can click-select it.
- **Loading:** a static `GatherResourceDependencies()` tells `SceneLoadOperation` which mesh a serialised `RenderComponent` needs, so it's loaded asynchronously before the object is created ([Asset-Import-Runtime.md](../05-AssetPipeline/Asset-Import-Runtime.md)).
- **Inspector** (described properties): Mesh, an asset path whose `...` button opens the [file browser](../07-Editor/Thumbnails-And-File-Browser.md) on the mesh thumbnails, and whose setter `LoadMesh(path)` loads it through the resource system; its **Size** in metres (mesh bounds × transform scale); "Draw Mesh Bounds" (yellow) and "Draw Sub-Mesh Bounds" (blue); and a Sub-Meshes tree with each submesh's material, shader and textures (with previews). See [Object-Inspector-And-Properties.md](../07-Editor/Object-Inspector-And-Properties.md).
- `GetWorldBounds()` and `GetRenderObject()` expose the world-space bounds for framing, marquee selection and mesh thumbnails. `GetLocalBounds()` (the mesh's box in the object's own space) feeds its `GameObject`'s hierarchy bounds.

## `CameraComponent`

A scene camera: FOV (degrees), near/far clip (0.1 m / 1000 m in the shipped scenes) and exposure compensation (EV).
- **Orientation comes from the owner's `TransformComponent` rotation**: −Z is forward, +Y is up. The Editor's rotate gizmo and the Rotation fields aim it; there's no separate look-at state.
- When it's the scene's **main camera** ("Main Camera" in the inspector, saved as `"MainCamera"`), `Update()` writes its exposure compensation, and its `ClipSpace` (projection × view, 0..1 depth) and position, into `RZE().GetRenderEngine().GetMainView()`. The matrices are skipped while `GameScene::DoesMainCameraDriveView()` is false, as in the Editor, whose own camera drives the view. A scene can have any number of cameras; only one is main. See [Scene-And-GameObject-Model.md](Scene-And-GameObject-Model.md#the-main-camera).
- `SetMainCamera(true)` makes it the main camera through `GameScene::SetMainCamera`, which clears the flag on the previous one; `SetMainCamera(false)` clears the role if it has it. The inspector property is `Exclusive`, so it can only be turned on (see [Object-Inspector-And-Properties.md](../07-Editor/Object-Inspector-And-Properties.md#metadata)). A pasted camera is never main.
- Only the **active scene's** main camera drives the main view; a camera in another scene (for example a thumbnail's `HeadlessScene`) never does.
- `CreateRenderCamera(viewportSize)` builds a `RenderCamera` from the camera at that size's aspect ratio without touching the camera itself. Scene thumbnails use it at 512×512.
- Owns a small render target and a secondary `RenderView`. `RenderPreview()` renders the **"Camera View" preview** into it with its own local projection, leaving the camera untouched; the editor shows it in a custom inspector section ([Engine-Graphics-Layer.md](../04-Rendering/Engine-Graphics-Layer.md#views)).
- Its inspector fields go through `SetFOV`, `SetNearCull`, `SetFarCull` and `SetExposureCompensation`. `SetFOV` and `SetNearCull` assert the value is positive, so the inspector's ranges keep them above 0. An active camera rebuilds its matrices every `Update()`, and the preview builds its own each time it renders, so changes show straight away.
- In the Editor, every scene camera shows a movie-camera icon and its frustum in the scene view.

## `DirectionalLightComponent`

Wraps a `LightObjectPtr` (strength + `Vector4D` colour, edited through `SetLightStrength` / `SetLightColour`). Creates the light object in its scene's `RenderScene` on `OnAddToScene()`, destroys it on `OnRemoveFromScene()`. Its **direction comes from the owner's transform rotation**; rotating it bumps the render scene's revision, so the shadow map re-renders. In the Editor it shows as a sun icon. See `LightObject` in [Engine-Graphics-Layer.md](../04-Rendering/Engine-Graphics-Layer.md).

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
| `DirectionalLightComponent` | `RZE_Application::RegisterEngineComponents()` | Yes | `RenderEngine` light objects |
| `PlayerControllerComponent` | not registered | No — attached at runtime | `InputHandler`, `TransformComponent` |
| `GifChatComponent` | `RZE_Application::RegisterEngineComponents()` | Yes | — |
| `EditorCameraComponent` | `Editor::EditorApp::Initialize()` | No — excluded via `SetIncludeInSave(false)` | `CameraComponent` (inheritance — buggy ID), `RenderEngine`, `InputHandler` |

## Key files

- `Engine\Src\Game\World\GameObjectComponents\TransformComponent.h/.cpp`
- `Engine\Src\Game\World\GameObjectComponents\RenderComponent.h/.cpp`
- `Engine\Src\Game\World\GameObjectComponents\CameraComponent.h/.cpp`
- `Engine\Src\Game\World\GameObjectComponents\DirectionalLightComponent.h/.cpp`
- `Engine\Src\Game\World\GameObjectComponents\PlayerControllerComponent.h/.cpp`
- `Engine\Src\Game\World\GameObjectComponents\GifChatComponent.h/.cpp`
- `Editor\Src\Game\World\GameObjectComponents\EditorCameraComponent.h/.cpp`
