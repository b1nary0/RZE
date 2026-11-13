// Mesh instances drawn for their depth alone (DepthOnlyMeshBatches: the depth prepass, the shadow maps): the vertices'
// positions and UVs in slot 0, and each instance's world transform in slot 1 (MeshInstanceFormats::Depth). Nothing
// for lighting: no normal, tangent or material values. Places vertices as Vertex_MeshInstanced does
// (MeshWorldPosition, MeshClipPosition), so the prepass's depths are the forward pass's to the bit.
#include "Common/MeshInstance.hlsli"
#include "Common/MeshVertex.hlsli"

DepthVertexToPixel VSMain(DepthVertexInput vertex, MeshDepthInstanceInput input)
{
	MeshWorld world;
	world.ColumnA = input.WorldA;
	world.ColumnB = input.WorldB;
	world.ColumnC = input.WorldC;
	world.ColumnD = input.WorldD;

	DepthVertexToPixel output;
	output.Position = MeshClipPosition(MeshWorldPosition(vertex.Position, world));
	output.UV = vertex.UV;
	return output;
}
