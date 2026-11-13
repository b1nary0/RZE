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
	// What a vertex shader drawn with it takes: the mesh vertex in slot 0, the instance in slot 1
	// (MeshInstanceFormats::k_instanceSlot)
	Rendering::ShaderInputLayout InputLayout;
	// Writes the draw's instance, as it is now, into Stride bytes at out
	void (*Fill)(const MeshDraw& draw, void* out) = nullptr;
};

namespace MeshInstanceFormats
{
	// Where every format's instances are read, beside the mesh's vertices in slot 0
	constexpr U32 k_instanceSlot = 1;

	// Everything a shaded draw needs besides its geometry and material: its matrices and the material values it
	// carries (a GraphicsLayouts::MeshInstanceLayout, MeshDraw::BuildInstance). Read by MeshInstanceInput.
	const MeshInstanceFormat& Forward();
}
