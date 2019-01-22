#include <StdAfx.h>
#include <Graphics/StaticMesh.h>

StaticMesh::StaticMesh()
{
}

StaticMesh::~StaticMesh()
{
}

void StaticMesh::SetMeshes(const std::vector<MeshGeometry>& meshGeometry)
{
	mSubMeshes = std::move(meshGeometry);
}

const std::vector<MeshGeometry>& StaticMesh::GetSubMeshes() const
{
	return mSubMeshes;
}

