#pragma once

#include <Graphics/MeshGeometry.h>

#include <Utils/PrimitiveDefs.h>

#include <Utils/Math/Vector2D.h>
#include <Utils/Math/Vector3D.h>

class StaticMesh
{
public:
	StaticMesh();
	~StaticMesh();

	void SetMeshes(const std::vector<MeshGeometry>& meshGeometry);
	const std::vector<MeshGeometry>& GetSubMeshes() const;

private:
	std::vector<MeshGeometry> mSubMeshes;
};