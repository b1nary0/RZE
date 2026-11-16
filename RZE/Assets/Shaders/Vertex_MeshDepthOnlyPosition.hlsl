// Depth-only passes (DepthOnlyMeshBatches: the depth prepass, the shadow maps), for draws that run no pixel shader: the
// vertices are only positions (MeshGeometry::DrawDepth), with each instance's world transform in slot 1
// (MeshInstanceFormats::DepthPosition). Vertex_MeshDepthOnly is the same for cutouts, which also need UVs. Places
// vertices as Vertex_MeshInstanced does (MeshWorldPosition, MeshClipPosition), so the prepass's depths are the forward
// pass's to the bit.
#include "Common/MeshInstance.hlsli"
#include "Common/MeshVertex.hlsli"

DepthPositionVertexToPixel VSMain(DepthPositionVertexInput vertex, MeshDepthInstanceInput input)
{
	MeshWorld world;
	world.ColumnA = input.WorldA;
	world.ColumnB = input.WorldB;
	world.ColumnC = input.WorldC;
	world.ColumnD = input.WorldD;

	DepthPositionVertexToPixel output;
	output.Position = MeshClipPosition(MeshWorldPosition(vertex.Position, world));
	return output;
}
