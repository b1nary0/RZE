#pragma once

#include <Utils/PrimitiveDefs.h>

#include <Utils/Math/Vector2D.h>
#include <Utils/Math/Vector3D.h>

class Material;

struct MeshVertex
{
	Vector3D Position;
	Vector3D Normal;
	Vector3D Tangent;
	Vector2D UVData;
};

class MeshGeometry
{
public:
	MeshGeometry();
	~MeshGeometry();

	void AddVertex(const MeshVertex& vertex);
	void AddIndex(U32 index);

	void SetMaterial(Material* material);

	const Material& GetMaterial() const;
	const std::vector<U32>& GetIndices() const;
	const std::vector<MeshVertex>& GetVertices();

	Diotima::GFXMesh* GetGPUMesh() const { return mGPUMesh; }

	void OnLoadFinished();

private:
	Material* mMaterial;

	// #TODO(Josh::Temp until better implementation. Just to get things working, then will push this down again)
	Diotima::GFXMesh* mGPUMesh;

	std::vector<MeshVertex> mVertices;
	std::vector<U32> mIndices;
};