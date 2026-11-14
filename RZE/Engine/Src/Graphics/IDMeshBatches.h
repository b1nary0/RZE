#pragma once

#include <Graphics/MeshBatches.h>
#include <Graphics/MeshInstanceFormat.h>

#include <Utils/PrimitiveDefs.h>

struct MeshDraw;

// A pass's mesh draws drawn to write an ID the stage gives each draw into its pixels, as picking and the outline mask
// draw them: in MeshInstanceFormats::ID, which carries the ID beside the draw's world transform, so the draws of one
// geometry are one batch whichever objects they're of. Batched as DepthOnlyMeshBatches does: a draw without an opacity
// map by its geometry alone, one with a map (a cutout) by its material too, as its mask is bound. The stage's pixel
// shader runs for every batch and clips by the mask (ApplyOpacityMask), so every batch binds its material's buffer,
// whose HasOpacityMask is 0 for a batch without a map whichever of its draws' materials it is.
//
// The stage sets up everything else: the target, Vertex_MeshID with MeshInstanceFormats::ID, its camera, its pixel
// shader and the depth, rasterizer and topology states. A pass: Clear, Add each draw, Draw.
class IDMeshBatches
{
public:
	IDMeshBatches() = default;
	~IDMeshBatches() = default;

	IDMeshBatches(const IDMeshBatches&) = delete;
	IDMeshBatches& operator=(const IDMeshBatches&) = delete;

public:
	// Makes the instance buffer
	void Initialize();

	// Forgets the last pass's draws
	void Clear();
	// A draw whose pixels get id: its low half in the shader's ID.x, its high half in ID.y. Blended draws are drawn
	// like any other. The draw must stay put until Draw.
	void Add(const MeshDraw& draw, U64 id);

	// Uploads the instances and draws every batch into the bound target. Returns how many draw calls it made.
	U32 Draw();

private:
	MeshBatches m_batches{ MeshInstanceFormats::ID() };
};
