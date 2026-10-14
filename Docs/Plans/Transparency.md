# Transparency and blending

Status: planned. Today the forward pass draws everything opaque, and the only "see-through" a material can do is a
cutout (`clip(mask - AlphaCutoff)` in `Assets/Shaders/Common/Lighting.hlsli`).

What that costs us right now, from burning Intel's New Sponza FBX:

- **Glass is opaque.** `glass`, `lamp_glass_01` and `light_bulb` come in with no textures and opacity 1. The source's
  transparency isn't read, and nothing could draw it see-through anyway.
- **Decals are hard-edged and too strong.** `dirt_decal` uses a soft mask plus a 0.39 transparency factor. We use the
  mask as a cutout (a stopgap: `AssimpSourceImporter::FindCutoutMaskFallback`, marked `#TODO(Transparency)`), so the
  dirt shows at full strength with jagged edges.

The work has two parts. Part 1 makes surfaces see-through (glass). Part 2 builds on it to blend surfaces over what's
behind them in other ways (soft decals, additive effects), and retires the cutout stopgap.

## What exists already

| Piece | Where | Use for this |
|---|---|---|
| `EBlendState::AlphaBlend` (straight alpha) | `Modules/Rendering/.../RenderingDefines.h`, `DX11Device.cpp` | The blend state the transparent pass needs. Only debug lines use it today. |
| `Opacity` in the material buffer | `PixelResources.hlsli` (`MaterialBuffer`), `Material.cpp` | Reaches the GPU already. No shader reads it. |
| Cutout path (`HasOpacityMask`, `OpacityMaskInAlpha`) | `Lighting.hlsli`, `MaterialAssetImporter.h` | Stays as one of the modes in Part 2. |
| Per-placement draws and bounds | `RenderObject::GetPlacementTransform`, `GetSubMeshBounds` | World-space centres for sorting transparent draws. |

What's missing: a depth state abstraction (no way to test depth without writing it), any notion of a material being
transparent, a transparent pass, and an importer that reads transparency.

## Part 1: Transparency

Goal: glass and other uniformly see-through surfaces draw blended, behind-to-front, after everything opaque.

### 1. Material data: a blend mode and a factor

- **`.materialasset` version bump.** Add `EBlendMode BlendMode` (`Opaque`, `Cutout`, `Transparent`; Part 2 adds more)
  and keep `Opacity` as the factor. Today's files are unversioned (their first field is the buffer size), so this
  needs a version field first. Old files read as `Opaque`, or as `Cutout` when they have an opacity texture.
- **Importer** (`AssimpSourceImporter::ProcessMesh`):
  - `Opacity` comes from `AI_MATKEY_OPACITY`, else `$mat.transparencyfactor` (as `1 - factor`), else 3ds Max's
    `$raw.3dsMax|Parameters|transparency` (as `1 - value`).
  - `Opacity < 1` gives `Transparent`, an opacity texture gives `Cutout`, otherwise `Opaque`.
  - Per-format quirks live in one function, so the Maya (Stingray/StandardSurface `opacity`, `transmission`) and
    Blender (glTF-style alpha) names can be added in one place.
- **Engine:** `MaterialInstance` carries `BlendMode`. The editor's material properties show it and let you change it,
  so a wrong import can be fixed by hand.

### 2. Depth states

- Add `EDepthState { ReadWrite, ReadOnly, Off }` next to `EBlendState`, with `Renderer::SetDepthState`, a
  `RenderCommand_SetDepthState` and the three D3D11 depth-stencil states in `DX11Device`.
- Transparent draws test depth (opaque geometry still hides them) but don't write it (glass shouldn't hide what's
  behind other glass).

### 3. The transparent pass

- `ForwardRenderStage` draws in two steps:
  1. **Opaque and cutout placements:** as today.
  2. **Transparent placements:** gathered into a list of `{ distance, render object, placement }` while walking the
     scene, sorted back to front by the distance from the camera to each placement's world-bounds centre. Then drawn
     with `AlphaBlend` and `ReadOnly` depth, and the blend and depth states restored afterwards.
- Sorting is per placement, not per triangle, so overlapping transparent parts of one mesh can still sort wrong.
  That's an accepted limit; order-independent transparency is out of scope.
- The pixel shaders output `alpha = Opacity` (times the albedo texture's alpha, if the texture has one) instead of 1.
  Done without the albedo alpha so far. `MaterialInstance::Create` keeps the GPU opacity at 1 for anything not
  `Transparent`, because the scene's alpha is also PostProcess's background marker and exposure weight.
- **Known gap:** a transparent surface over empty background (scene alpha 0) vanishes. `AlphaBlend` leaves the
  target's alpha alone, so it stays 0 and `Pixel_Tonemap.hlsl` paints the background colour over it. Fix by clearing
  the scene target to the (linear) background colour and treating alpha as coverage, or by a blend state for this
  pass that accumulates alpha, with `Pixel_Tonemap`/`Pixel_LuminanceLog` updated to match.
- The transparent list is built during the existing walk over render objects, so it needs no extra pass over the
  scene. It's cleared every frame.

### 4. Other stages

- **Shadows** (`ShadowRenderStage`): transparent placements don't cast, as a first step. Clear glass casting a solid
  shadow looks worse than no shadow. Coloured or partial shadows are future work.
- **Picking** (`PickRenderStage`): transparent placements stay pickable. They're drawn into the ID buffer like opaque
  ones, so you can click a window.
- **Thumbnails** render through the same stages, so they follow automatically.

### 5. Checks

- Unit tests: back-to-front sort order; importer mapping of opacity and transparency factor into a blend mode.
- New Sponza: the windows show the courtyard through them, and the shadows under them are clear of glass.
- Sponza (OBJ): looks the same. All 25 of its materials have `d 1.0`, so it should take only the opaque and cutout
  paths. A/B
  performance, since every frame now builds and sorts a (normally empty) list.

## Part 2: Blending

Goal: surfaces blend over what's behind them in more ways than "see-through", starting with soft decals. The cutout
stopgap goes away.

### 1. More blend modes

| Mode | Blend | Typical use |
|---|---|---|
| `AlphaBlend` (from Part 1's `Transparent`) | `src * a + dst * (1 - a)` | Glass, soft decals |
| `Premultiplied` | `src + dst * (1 - a)` | Textures authored premultiplied, cleaner mip edges |
| `Additive` | `src * a + dst` | Glows, light shafts, emissive cards |
| `Multiply` | `src * dst` | Dirt and grime that only darkens |

- Add the matching `EBlendState`s and D3D11 states. `BlendMode` in the material asset picks one. A blended material
  draws in the transparent pass, sorted like Part 1.

### 2. Soft masks and decals

- **Mask channel:** the opacity texture becomes a *coverage* input to blending, not only a cutout.
  `alpha = Opacity * mask`, and `Cutout` remains its own mode (`clip`, no blending).
- **Mask conventions:** an `InvertMask` flag in the material, set by the importer when the source says the map is
  inverted, or by hand in the editor.
- **Decals that sit on a surface** (New Sponza's `dirt_decal` geometry) z-fight with the wall behind them once they
  stop writing depth. Give blended decals a small depth bias, via a rasterizer state like `ShadowCaster` but with a
  negative bias, so they win the depth test against the surface they're laid on.
- **Lighting:** decals are lit like the surface under them. They use the same pixel shader, only blended.

### 3. Retire the stopgap

- `FindCutoutMaskFallback` becomes the importer's normal reading of 3ds Max's `cutout_map` (as `Cutout`) and
  `transparency_map` (as an `AlphaBlend` coverage mask). The `#TODO(Transparency)` markers come out.
- Re-burn New Sponza. The dirt should read as soft grime at the source's 0.39 strength, not dark patches.

### 4. Checks

- Unit tests: each blend mode's importer mapping; mask inversion.
- New Sponza: decal edges are soft, decals don't flicker against walls (depth bias), and the strength matches the
  source's factor.
- The cutout path still works: Sponza (OBJ) thorn leaves, vase plants and chains keep their cutouts (its only three
  `map_d` materials).

## Order of work

Each step is one commit, and each leaves the engine working.

1. Material asset version field, then `BlendMode` + factor (format, importer, engine, editor).
2. `EDepthState` in the renderer.
3. Transparent pass with sorting; shaders output alpha; shadow and pick handling.
4. Importer: read transparency from FBX and 3ds Max properties (Part 1 done).
5. Additional blend states and modes.
6. Soft masks, `InvertMask`, decal depth bias.
7. Retire `FindCutoutMaskFallback`; re-burn and compare (Part 2 done).

## Open questions

- Should a transparent material with an albedo texture take its alpha from that texture by default, or only when told?
  OBJ's `map_d == map_Kd` convention says "told". glTF says "always".
- Do we want order-independent transparency (weighted blended OIT) eventually? It would replace per-placement sorting
  but needs an extra render target and composite. Not needed for glass in architecture scenes.
