#include <StdAfx.h>
#include <Graphics/MeshGeometry.h>

#include <Graphics/Material.h>

#include <Diotima/Graphics/GFXMesh.h>

MeshGeometry::MeshGeometry()
{
}

MeshGeometry::~MeshGeometry()
{
}

void MeshGeometry::AddVertex(const MeshVertex& vertex)
{
	mVertices.push_back(vertex);
}

void MeshGeometry::AddIndex(U32 index)
{
	mIndices.push_back(index);
}

void MeshGeometry::SetMaterial(Material* material)
{
	AssertNotNull(material);
	mMaterial = material;
}

const Material& MeshGeometry::GetMaterial() const
{
	AssertNotNull(mMaterial);
	return *mMaterial;
}

const std::vector<U32>& MeshGeometry::GetIndices() const
{
	return mIndices;
}

const std::vector<MeshVertex>& MeshGeometry::GetVertices()
{
	return mVertices;
}

void MeshGeometry::OnLoadFinished()
{
	mGPUMesh = new Diotima::GFXMesh();

	std::vector<void*> vertices;
	vertices.reserve(mVertices.size());
	for (MeshVertex& vertex : mVertices)
	{
		vertices.push_back(static_cast<void*>(&vertex));
	}

	mGPUMesh->Allocate(vertices, mIndices);
}
