#include <StdAfx.h>
#include <Graphics/StaticMesh.h>

#include <Graphics/Material.h>

StaticMesh::StaticMesh()
{
}

StaticMesh::~StaticMesh()
{
}

void StaticMesh::AddVertex(const MeshVertex& vertex)
{
	mVertices.push_back(vertex);
}

void StaticMesh::AddIndex(U32 index)
{
	mIndices.push_back(index);
}

void StaticMesh::SetMaterial(Material* material)
{
	AssertNotNull(material);
	mMaterial = material;
}

const Material& StaticMesh::GetMaterial() const
{
	AssertNotNull(mMaterial);
	return *mMaterial;
}

const std::vector<U32>& StaticMesh::GetIndices() const
{
	return mIndices;
}

const std::vector<MeshVertex>& StaticMesh::GetVertices()
{
	return mVertices;
}

void StaticMesh::OnLoadFinished()
{
}
