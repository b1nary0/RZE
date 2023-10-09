#pragma once

#include <Graphics/MeshGeometry.h>

class StaticMeshInstance
{
public:
	StaticMeshInstance();
	~StaticMeshInstance();

	void Initialize(const std::vector<MeshGeometry>& meshGeometry);

	std::vector<MeshGeometry>& GetSubMeshes();
	const std::vector<MeshGeometry>& GetSubMeshes() const;

	const std::string& GetName() const;
	void SetName(const std::string& name);

private:
	std::vector<MeshGeometry> m_subMeshes;
	std::string m_name;
};