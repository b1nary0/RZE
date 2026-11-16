#pragma once

#include <Rendering/Driver/ShaderTypes.h>

#include <Utils/PrimitiveDefs.h>

struct MeshDraw;

// One kind of mesh instance, as a pass's instance stream holds it (MeshInstanceStream): its size, how a vertex shader
// reads it, and how one is built from a draw. Each pass takes only what its shaders read, so each kind is a format
// (MeshInstanceFormats). The C++ structs they fill are in GraphicsLayouts.h, and the shaders' in
// Common/MeshInstance.hlsli.
struct MeshInstanceFormat
{
	// One instance's size in bytes, and the stream's stride
	U32 Stride = 0;
	// What a vertex shader drawn with it takes: the mesh vertex, or the part of it the shader reads, in slot 0, the
	// instance in slot 1 (MeshInstanceFormats::k_instanceSlot)
	Rendering::ShaderInputLayout InputLayout;
	// Writes the draw's instance, as it is now, into Stride bytes at out. id is what the stage gave the draw
	// (MeshBatches::Add) for a format that CarriesID, and 0 for one that doesn't.
	void (*Fill)(const MeshDraw& draw, U64 id, void* out) = nullptr;
	// Whether each instance carries an ID the stage gives its draw, which nothing in the draw knows (which object it is)
	bool CarriesID = false;
};

namespace MeshInstanceFormats
{
	// Where every format's instances are read, beside the mesh's vertices in slot 0
	constexpr U32 k_instanceSlot = 1;

	// Everything a shaded draw needs besides its geometry and material: its matrices and the material values it
	// carries (a GraphicsLayouts::MeshInstanceLayout, MeshDraw::BuildInstance). Read by MeshInstanceInput.
	const MeshInstanceFormat& Forward();
	// What a depth-only pass needs: the vertex's position and UV (for cutouts' masks), and the draw's world transform
	// (a GraphicsLayouts::MeshDepthInstanceLayout), built without its material slot. Read by MeshDepthInstanceInput.
	const MeshInstanceFormat& Depth();
	// Depth's instances, read with only the vertex's position: for the geometry's depth buffers
	// (MeshGeometry::DrawDepth), which hold nothing else. Streams as Depth does, so a MeshBatches of Depth's can
	// feed it. Read by MeshDepthInstanceInput.
	const MeshInstanceFormat& DepthPosition();
	// What a pass that writes an ID for each draw's pixels needs: Depth's, and the ID the stage gave the draw (a
	// GraphicsLayouts::MeshIDInstanceLayout), built without its material slot. Read by MeshIDInstanceInput.
	const MeshInstanceFormat& ID();
}
