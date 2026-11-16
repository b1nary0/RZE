#pragma once

#include <Graphics/MeshBatches.h>
#include <Graphics/MeshInstanceFormat.h>

#include <EngineCore/Resources/TypedResourceHandle.h>

#include <Utils/PrimitiveDefs.h>

class PixelShader;
class VertexShader;
struct MeshDraw;

// A pass's mesh draws drawn for their depth alone, as the depth prepass and the shadow maps draw them, so the two
// can't drift apart. A draw without an opacity map binds nothing but its geometry and runs no pixel shader, so every
// draw of a geometry is one batch whatever its material, and it's drawn from the geometry's depth buffers (positions
// only, welded, in vertex-cache order: MeshGeometry::DrawDepth) with Vertex_MeshDepthOnlyPosition. One with an opacity
// map (a cutout: leaves, chains) runs a pixel shader that only clips, with its material's mask bound, so it batches by
// material too, and it's drawn from the full vertices (it needs their UVs) with Vertex_MeshDepthOnly. Both vertex
// shaders place vertices the same way, to the bit as the forward pass does.
//
// The stage sets up everything else: the target, the camera in the vertex stage's constant buffer slot 0 (both vertex
// shaders read it there) and the depth, rasterizer and topology states. This binds the vertex shaders and their
// input layouts. A pass: Clear, Add each draw, Draw.
class DepthOnlyMeshBatches
{
public:
	DepthOnlyMeshBatches() = default;
	~DepthOnlyMeshBatches() = default;

	DepthOnlyMeshBatches(const DepthOnlyMeshBatches&) = delete;
	DepthOnlyMeshBatches& operator=(const DepthOnlyMeshBatches&) = delete;

public:
	// Loads the shaders and makes the instance buffer
	void Initialize();

	// Forgets the last pass's draws
	void Clear();
	// A draw whose depth is wanted. Not for blended draws: what they cover still shows, so they write no depth and
	// the stage leaves them out. The draw must stay put until Draw.
	void Add(const MeshDraw& draw);

	// Uploads the instances and draws every batch into the bound depth target. A vertex shader and its input layout are
	// bound once for each run of opaque batches or of cutouts. Leaves the pixel shader unset, or the cutout one bound
	// if the last batch was a cutout, and the vertex shader of the last batch bound. Returns how many draw calls it made.
	U32 Draw();

private:
	// Vertex_MeshDepthOnlyPosition: for opaque draws, from the depth buffers
	TypedResourceHandle<VertexShader> m_opaqueVertexShader;
	// Vertex_MeshDepthOnly: for cutouts, from the full vertices
	TypedResourceHandle<VertexShader> m_cutoutVertexShader;
	// Only clips; bound just for cutouts
	TypedResourceHandle<PixelShader> m_cutoutShader;

	// Streams the instances both vertex shaders read (Depth and DepthPosition's instances are the same)
	MeshBatches m_batches{ MeshInstanceFormats::Depth() };
};
