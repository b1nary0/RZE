#include "Common/MeshVertex.hlsli"

// The draw's transform from the ObjectBuffer, uploaded before each draw. The depth prepass and forward pass draw with
// Vertex_MeshInstanced instead.
VertexToPixel VSMain(VertexInput input)
{
	return TransformMeshVertex(input, World, InvWorld);
}
