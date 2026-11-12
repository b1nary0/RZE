#include "Common/MeshVertex.hlsli"

// The draw's transform from the ObjectBuffer, uploaded before each draw: picking and outlines, whose pixel shaders
// read where a draw is and its opacity mask, never its tint, shininess or opacity, so those are neutral here. The
// depth prepass, shadow and forward passes draw with Vertex_MeshInstanced instead.
VertexToPixel VSMain(VertexInput input)
{
	return TransformMeshVertex(input, World, InvWorld, NeutralInstanceMaterial());
}
