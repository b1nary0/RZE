#include "Common/MeshVertex.hlsli"

// The draw's transform from the ObjectBuffer, uploaded before each draw: picking and outlines, whose pixel shaders
// read where a draw is and its opacity mask, never its tint, shininess or opacity, so those are neutral here. The
// forward pass draws with Vertex_MeshInstanced instead, and the depth prepass and shadows with Vertex_MeshDepthOnly.
VertexToPixel VSMain(VertexInput input)
{
	return TransformMeshVertex(input, World, InvWorld, NeutralInstanceMaterial());
}
