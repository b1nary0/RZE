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

	// Object-space axis-aligned bounds enclosing every sub-mesh; calculated in Initialize()
	const Vector3D& GetBoundsMin() const { return m_boundsMin; }
	const Vector3D& GetBoundsMax() const { return m_boundsMax; }

private:
	void CalculateBounds();

private:
	std::vector<MeshGeometry> m_subMeshes;
	std::string m_name;
	Vector3D m_boundsMin;
	Vector3D m_boundsMax;
};