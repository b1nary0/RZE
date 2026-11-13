#pragma once

#include <Graphics/MeshBatches.h>
#include <Graphics/MeshInstanceFormat.h>

#include <EngineCore/Resources/TypedResourceHandle.h>

#include <Utils/PrimitiveDefs.h>

class PixelShader;
struct MeshDraw;

// A pass's mesh draws drawn for their depth alone, as the depth prepass and the shadow maps draw them, so the two
// can't drift apart. A draw without an opacity map binds nothing but its geometry and runs no pixel shader, so every
// draw of a geometry is one batch whatever its material. One with an opacity map (a cutout: leaves, chains) runs a
// pixel shader that only clips, with its material's mask bound, so it batches by material too.
//
// The stage sets up everything else: the target, Vertex_MeshInstanced with MeshInstanceFormats::Forward, its camera and
// the depth, rasterizer and topology states. A pass: Clear, Add each draw, Draw.
class DepthOnlyMeshBatches
{
public:
	DepthOnlyMeshBatches() = default;
	~DepthOnlyMeshBatches() = default;

	DepthOnlyMeshBatches(const DepthOnlyMeshBatches&) = delete;
	DepthOnlyMeshBatches& operator=(const DepthOnlyMeshBatches&) = delete;

public:
	// Loads the cutout shader and makes the instance buffer
	void Initialize();

	// Forgets the last pass's draws
	void Clear();
	// A draw whose depth is wanted. Not for blended draws: what they cover still shows, so they write no depth and
	// the stage leaves them out. The draw must stay put until Draw.
	void Add(const MeshDraw& draw);

	// Uploads the instances and draws every batch into the bound depth target. Leaves the pixel shader unset, or the
	// cutout one bound if the last batch was a cutout. Returns how many draw calls it made.
	U32 Draw();

private:
	// Only clips; bound just for cutouts
	TypedResourceHandle<PixelShader> m_cutoutShader;

	MeshBatches m_batches{ MeshInstanceFormats::Forward() };
};
