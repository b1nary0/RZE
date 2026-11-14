// Mesh instances drawn to write an ID the stage gave each draw into their pixels (IDMeshBatches: picking, the outline
// mask): the vertices' positions and UVs in slot 0, and each instance's world transform and ID in slot 1
// (MeshInstanceFormats::ID). Places vertices as Vertex_MeshDepthOnly does (MeshWorldPosition, MeshClipPosition), so
// the outline mask's depths are the prepass's to the bit.
#include "Common/MeshInstance.hlsli"
#include "Common/MeshVertex.hlsli"

IDVertexToPixel VSMain(DepthVertexInput vertex, MeshIDInstanceInput input)
{
	MeshWorld world;
	world.ColumnA = input.WorldA;
	world.ColumnB = input.WorldB;
	world.ColumnC = input.WorldC;
	world.ColumnD = input.WorldD;

	IDVertexToPixel output;
	output.Position = MeshClipPosition(MeshWorldPosition(vertex.Position, world));
	output.UV = vertex.UV;
	output.ID = uint2(input.IDLow, input.IDHigh);
	return output;
}
