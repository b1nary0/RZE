#ifndef MESH_VERTEX_HLSLI
#define MESH_VERTEX_HLSLI

#include "Common/ForwardTypes.hlsli"
#include "Common/VertexResources.hlsli"

// A mesh vertex placed in the world by its draw's transform and seen by the camera, with the material values its draw
// carries. Shared by the shaders that take the transform from the ObjectBuffer (Vertex_NewRenderer) and from the
// instance stream (Vertex_MeshInstanced), so the two give the same positions.
VertexToPixel TransformMeshVertex(VertexInput input, float4x4 world, float4x4 invWorld, InstanceMaterial material)
{
	VertexToPixel output;

	float4 worldPos = mul(world, float4(input.Position, 1.0f));

	output.Position = mul(ViewProjection, worldPos);
	output.WorldPos = worldPos.xyz;
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
