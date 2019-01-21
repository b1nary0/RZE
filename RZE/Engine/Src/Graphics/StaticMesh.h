#pragma once

#include <Graphics/MeshGeometry.h>

#include <Utils/PrimitiveDefs.h>

#include <Utils/Math/Vector2D.h>
#include <Utils/Math/Vector3D.h>

class StaticMesh
{
public:
	StaticMesh(const std::vector<MeshGeometry>& subMeshes);
	~StaticMesh();

	void SetMeshes(const std::vector<MeshGeometry>& meshGeometry);
	const std::vector<MeshGeometry>& GetSubMeshes();

private:
	std::vector<MeshGeometry> mSubMeshes;
};