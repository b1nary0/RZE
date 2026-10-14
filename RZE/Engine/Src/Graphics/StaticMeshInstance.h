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

	// Object-space box around every sub-mesh, calculated in Initialize(); a zero-sized box at the origin with none
	const BoundingBox& GetBounds() const { return m_bounds; }

private:
	void CalculateBounds();

private:
	std::vector<MeshGeometry> m_subMeshes;
	std::string m_name;
	BoundingBox m_bounds;
};