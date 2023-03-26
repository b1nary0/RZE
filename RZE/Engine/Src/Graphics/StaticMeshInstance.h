#pragma once

#include <Graphics/MeshGeometry.h>

class StaticMeshInstance
{
public:
	StaticMeshInstance();
	~StaticMeshInstance();

	void Initialize(const std::vector<MeshGeometry>&& meshGeometry);

	std::vector<MeshGeometry>& GetSubMeshes();
	const std::vector<MeshGeometry>& GetSubMeshes() const;

	Vector3D m_centerPos;

private:
	std::vector<MeshGeometry> m_subMeshes;
};