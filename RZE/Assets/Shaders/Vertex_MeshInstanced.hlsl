// Mesh instances (MeshBatches): the vertices in slot 0, and each instance's transform and material values in slot 1
// (MeshInstanceFormats::Forward), so a batch of instances is one draw. The depth prepass and forward pass draw with
// it, so both compute the same depths.
#include "Common/MeshInstance.hlsli"
#include "Common/MeshVertex.hlsli"

VertexToPixel VSMain(VertexInput vertex, MeshInstanceInput input)
{
	const MeshInstance instance = ReadMeshInstance(input);
	return TransformMeshVertex(vertex, instance.World, instance.InvWorld, UnpackInstanceMaterial(instance.MaterialA, instance.MaterialB));
}
