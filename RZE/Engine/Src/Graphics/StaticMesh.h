#pragma once

#include <Utils/PrimitiveDefs.h>

#include <Utils/Math/Vector2D.h>
#include <Utils/Math/Vector3D.h>

class Material
{

};

struct MeshVertex
{
	Vector3D Position;
	Vector3D Normal;
	Vector3D Tangent;
	Vector2D UVData;
};

class StaticMesh
{
public:
	StaticMesh();
	~StaticMesh();

	void AddVertex(const MeshVertex& vertex);
	void AddIndex(U32 index);

	void SetMaterial(Material* material);

	const Material& GetMaterial() const;
	const std::vector<U32>& GetIndices() const;
	const std::vector<Vector3D>& GetPositions() const;
	const std::vector<Vector3D>& GetNormals() const;
	const std::vector<Vector2D>& GetUVCoords() const;

	void OnLoadFinished();
private:
	Material* mMaterial;

	std::vector<Vector3D> mPositions;
	std::vector<Vector3D> mNormals;
	std::vector<Vector3D> mTangents;
	std::vector<Vector2D> mUVCoords;

	std::vector<U32> mIndices;
};