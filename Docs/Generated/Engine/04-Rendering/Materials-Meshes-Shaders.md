# Materials, Meshes & Shaders

This page covers the engine-side asset representation that sits between a burned `.meshasset`/`.materialasset` file on disk ([Asset-Burning-Pipeline.md](../05-AssetPipeline/Asset-Burning-Pipeline.md)) and the GPU handles consumed by `Rendering::Renderer` ([Low-Level-Renderer-Module.md](Low-Level-Renderer-Module.md)).

## Asset representation hierarchy (reproduced from `DrawIO\AssetRepresentation_March2021.drawio`)

```mermaid
%%{init: {'flowchart': {'subGraphTitleMargin': {'top': 6, 'bottom': 10}}, 'themeCSS': '.labelBkg, .edgeLabel { background-color: var(--background-primary) !important; } .messageText, .labelText, .loopText { paint-order: stroke; stroke: var(--background-primary); stroke-width: 5px; stroke-linejoin: round; }'}}%%
flowchart TD
    Asset[Asset] --> Model3D[Model3D]
    Model3D --> StaticMesh[StaticMesh / StaticMeshInstance]
    StaticMesh --> MG1[MeshGeometry #1]
    StaticMesh --> MG2[MeshGeometry #2]
    StaticMesh --> MG3[MeshGeometry #3]
    MG1 --> Mat1[Material]
    MG2 --> Mat2[Material]
    MG3 --> Mat3[Material]
    Mat1 --> MP1[MaterialParams]
    Mat1 --> Tex1["Resource<Texture2D>"]
    Mat2 --> MP2[MaterialParams]
    Mat2 --> Tex2["Resource<Texture2D>"]
    Mat3 --> MP3[MaterialParams]
    Mat3 --> Tex3["Resource<Texture2D>"]
```

A `StaticMeshInstance` is a named collection of `MeshGeometry` sub-meshes (one per Assimp mesh node at import time); each sub-mesh owns exactly one `MaterialInstance`, and each material owns its own `MaterialParams` plus texture resource(s).

## Shader GPU resource grouping (reproduced from page 2 of the same `.drawio`)

```mermaid
%%{init: {'flowchart': {'subGraphTitleMargin': {'top': 6, 'bottom': 10}}, 'themeCSS': '.labelBkg, .edgeLabel { background-color: var(--background-primary) !important; } .messageText, .labelText, .loopText { paint-order: stroke; stroke: var(--background-primary); stroke-width: 5px; stroke-linejoin: round; }'}}%%
flowchart TD
    Shader --> GPURes["GPU Resources"]
    GPURes --> CB[ConstantBuffers]
    GPURes --> TB[TextureBuffers]
    GPURes --> GSO["GPU Shader Object"]
```

## `MeshGeometry` (`Engine\Src\Graphics\MeshGeometry.h` / `.cpp`)

Per-submesh geometry: `MeshVertex { Position, Normal, Tangent, UVData }`, plus vertex/index arrays and their GPU-side `VertexBuffer`/`IndexBuffer` (`shared_ptr`), and an associated `MaterialInstance`.

## `StaticMeshInstance` (`Engine\Src\Graphics\StaticMeshInstance.h` / `.cpp`)

A named collection of `MeshGeometry` sub-meshes — this is the in-memory "asset" a `RenderObject` references. Loaded/populated via `StaticMeshResource` ([Asset-Import-Runtime.md](../05-AssetPipeline/Asset-Import-Runtime.md)).

## `MaterialInstance` (`Engine\Src\Graphics\Material.h` / `.cpp`)

Built from a burned `MaterialAssetImporter::MaterialData`. It holds a name, five texture slots, a pixel shader and a small constant buffer of parameters.

| Texture slot | Shader register | Used for |
|---|---|---|
| `TEXTURE_SLOT_DIFFUSE` | `t0` `DiffuseMap` | Base colour |
| `TEXTURE_SLOT_SPECULAR` | `t1` `SpecularMap` | Specular strength |
| `TEXTURE_SLOT_NORMAL` | `t2` `NormalMap` | Tangent-space normals |
| `TEXTURE_SLOT_OPACITY` | `t3` `OpacityMap` | Cutout mask (leaves, chains, lashes), in red or alpha |
| `TEXTURE_SLOT_HEIGHT` | `t4` `HeightMap` | Grayscale height, turned into extra normal detail |

`MaterialParams` (constant buffer `b1`, mirrored by `MaterialBuffer` in `Common/PixelResources.hlsli`): `Shininess`, `Opacity`, and the flags `HasOpacityMask`, `HasHeightMap` and `OpacityMaskInAlpha`. The flags matter because texture slots are **not unbound** between draws: without them a shader could sample the previous draw's texture.

**Which pixel shader a material gets** is decided from its texture flags when it's loaded:

```mermaid
%%{init: {'flowchart': {'subGraphTitleMargin': {'top': 6, 'bottom': 10}}, 'themeCSS': '.labelBkg, .edgeLabel { background-color: var(--background-primary) !important; } .messageText, .labelText, .loopText { paint-order: stroke; stroke: var(--background-primary); stroke-width: 5px; stroke-linejoin: round; }'}}%%
flowchart TD
    M["Material's texture flags"] --> Q1{"diffuse + specular<br/>+ normal?"}
    Q1 -->|yes| Full["Pixel_NewRenderer.hlsl"]
    Q1 -->|no| Q2{"no textures<br/>at all?"}
    Q2 -->|yes| Plain["Pixel_Default_NewRenderer.hlsl"]
    Q2 -->|no| Diff["Pixel_NewRenderer_DiffuseOnly.hlsl"]
```

Opacity and height maps work with any of the three, through the flags above. The loaded shader and textures are **resolved when they're set** (`GetPixelShader()`, `GetTextureResource(slot)`), so drawing never looks a resource up by path.

The same opacity mask is honoured in three passes: the forward pass, the shadow caster (cut-out shadows) and the pick pass (clicks go through cut-out pixels).

## `Shader` / `Texture2D` / `VertexBuffer` / `IndexBuffer` (`Engine\Src\Graphics\`)

Thin engine-side `IResource` wrappers around the lower-level GPU objects exposed by `Modules\Rendering` (`Rendering::VertexShaderHandle`/`PixelShaderHandle`, `Rendering::TextureBuffer2DHandle`, `Rendering::VertexBufferHandle`/`IndexBufferHandle`). These are what `ResourceHandler` loads/caches by file path.

## Key files

- `Engine\Src\Graphics\MeshGeometry.h/.cpp`
- `Engine\Src\Graphics\StaticMeshInstance.h/.cpp`
- `Engine\Src\Graphics\Material.h/.cpp`
- `Engine\Src\Graphics\Shader.h/.cpp`, `Texture2D.h/.cpp`, `VertexBuffer.h/.cpp`, `IndexBuffer.h/.cpp`
- `Engine\Src\Game\World\GameObjectComponents\StaticMeshResource.h/.cpp`
