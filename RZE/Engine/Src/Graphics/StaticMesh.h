#pragma once

#include <Graphics/MeshGeometry.h>

#include <Utils/PrimitiveDefs.h>

class StaticMesh
{
public:
	StaticMesh();
	~StaticMesh();

	void Initialize(const std::vector<MeshGeometry>& meshGeometry);

	const std::vector<MeshGeometry>& GetSubMeshes() const;

private:
	std::vector<MeshGeometry> mSubMeshes;
};