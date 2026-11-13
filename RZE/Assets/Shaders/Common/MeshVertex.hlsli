#ifndef MESH_VERTEX_HLSLI
#define MESH_VERTEX_HLSLI

#include "Common/ForwardTypes.hlsli"
#include "Common/VertexResources.hlsli"

// A draw's world transform as its four columns' first three values: transforms are affine, so the fourth row is always
// 0, 0, 0, 1. What MeshInstanceFormats::Depth streams (MeshDepthInstanceInput), and what every mesh vertex shader
// places its vertices with (MeshWorldPosition).
struct MeshWorld
{
	float3 ColumnA;
	float3 ColumnB;
	float3 ColumnC;
	float3 ColumnD;
};

MeshWorld GetMeshWorld(float4x4 world)
{
	MeshWorld columns;
	columns.ColumnA = float3(world._m00, world._m10, world._m20);
	columns.ColumnB = float3(world._m01, world._m11, world._m21);
	columns.ColumnC = float3(world._m02, world._m12, world._m22);
	columns.ColumnD = float3(world._m03, world._m13, world._m23);
	return columns;
}

// Where a mesh vertex is in the world, and in the camera's clip space. Every mesh vertex shader places its vertices
// with these two, so the depth prepass (Vertex_MeshDepthOnly) and the forward pass (Vertex_MeshInstanced) give the
// same depths to the bit: forward tests LessEqual against the prepass's depth and writes none, so a depth one ULP
// nearer in the prepass would drop the pixel. precise keeps the compiler from fusing or reordering the arithmetic, so
// it's done as written in both; the GL translator also marks the position invariant (GLShaderTranslator).
float3 MeshWorldPosition(float3 position, MeshWorld world)
{
	precise float3 worldPos = world.ColumnA * position.x + world.ColumnB * position.y + world.ColumnC * position.z + world.ColumnD;
	return worldPos;
}

float4 MeshClipPosition(float3 worldPos)
{
	precise float4 clipPos = mul(ViewProjection, float4(worldPos, 1.0f));
	return clipPos;
}

// A mesh vertex placed in the world by its draw's transform and seen by the camera, with the material values its draw
// carries. Shared by the shaders that take the transform from the ObjectBuffer (Vertex_NewRenderer) and from the
// instance stream (Vertex_MeshInstanced), so the two give the same positions.
VertexToPixel TransformMeshVertex(VertexInput input, float4x4 world, float4x4 invWorld, InstanceMaterial material)
{
	VertexToPixel output;

	const float3 worldPos = MeshWorldPosition(input.Position, GetMeshWorld(world));

	output.Position = MeshClipPosition(worldPos);
	output.WorldPos = worldPos;
	// Normals transform by the inverse-transpose so non-uniform scale doesn't skew them
	output.Normal = normalize(mul(input.Normal, (float3x3)invWorld));
	output.Tangent = normalize(mul((float3x3)world, input.Tangent));
	output.UV = input.UV;
	output.CameraPos = CameraPosition;
	output.Tint = material.Tint;
	output.Shininess = material.Shininess;
	output.Opacity = material.Opacity;

	return output;
}

#endif // MESH_VERTEX_HLSLI
