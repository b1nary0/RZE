// Mesh instances (MeshBatches): the vertices in slot 0, and each instance's transform and material values in slot 1
// (MeshInstanceFormats::Forward), so a batch of instances is one draw. The forward pass draws with it; its depths are
// the depth prepass's (Vertex_MeshDepthOnly) to the bit, as both place vertices with MeshClipPosition.
#include "Common/MeshInstance.hlsli"
#include "Common/MeshVertex.hlsli"

VertexToPixel VSMain(VertexInput vertex, MeshInstanceInput input)
{
	const MeshInstance instance = ReadMeshInstance(input);
	return TransformMeshVertex(vertex, instance.World, instance.InvWorld, UnpackInstanceMaterial(instance.MaterialA, instance.MaterialB));
}
