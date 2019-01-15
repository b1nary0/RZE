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

const std::vector<Vector3D>& StaticMesh::GetPositions() const
{
	return mPositions;
}

const std::vector<Vector3D>& StaticMesh::GetNormals() const
{
	return mNormals;
}

const std::vector<Vector2D>& StaticMesh::GetUVCoords() const
{
	return mUVCoords;
}

void StaticMesh::OnLoadFinished()
{
	for (size_t vertIdx = 0; vertIdx < mVertices.size(); vertIdx++)
	{
		mPositions.push_back(mVertices[vertIdx].Position);
		mNormals.push_back(mVertices[vertIdx].Normal);
		mUVCoords.push_back(mVertices[vertIdx].UVData);
		mTangents.push_back(mVertices[vertIdx].Tangent);
	}
}
