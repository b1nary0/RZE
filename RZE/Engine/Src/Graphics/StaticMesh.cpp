#include <StdAfx.h>
#include <Graphics/StaticMesh.h>

#include <Graphics/IndexBuffer.h>
#include <Graphics/VertexBuffer.h>

StaticMesh::StaticMesh()
{
}

StaticMesh::~StaticMesh()
{
}

void StaticMesh::Initialize(const std::vector<float>& meshData, const std::vector<U32>& indexData)
{
	mVertexBuffer = std::make_unique<VertexBuffer>(meshData);
	mVertexBuffer->Initialize();

	mIndexBuffer = std::make_unique<IndexBuffer>(indexData);
	mIndexBuffer->Initialize();
}

void StaticMesh::SetMeshes(const std::vector<MeshGeometry>& meshGeometry)
{
	mSubMeshes = std::move(meshGeometry);
}

const std::vector<MeshGeometry>& StaticMesh::GetSubMeshes() const
{
	return mSubMeshes;
}

