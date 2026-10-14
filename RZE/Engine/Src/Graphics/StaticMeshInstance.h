#pragma once

#include <Graphics/MeshGeometry.h>

class StaticMeshInstance
{
public:
	StaticMeshInstance();
	~StaticMeshInstance();

	// Places each sub-mesh once, at the mesh's origin
	void Initialize(const std::vector<MeshGeometry>& meshGeometry);
	// Each placement draws one sub-mesh with its own transform; a sub-mesh can be placed more than once
	void Initialize(const std::vector<MeshGeometry>& meshGeometry, std::vector<MeshPlacement>&& placements);

	// Each sub-mesh's geometry and material, once however many times it's placed
	std::vector<MeshGeometry>& GetSubMeshes();
	const std::vector<MeshGeometry>& GetSubMeshes() const;

	// What gets drawn: one entry per draw, indexing GetSubMeshes()
	const std::vector<MeshPlacement>& GetPlacements() const { return m_placements; }

	const std::string& GetName() const;
	void SetName(const std::string& name);

	// Object-space box around every placed sub-mesh, calculated in Initialize(); a zero-sized box at the origin with none
	const BoundingBox& GetBounds() const { return m_bounds; }

private:
	void CalculateBounds();

private:
	std::vector<MeshGeometry> m_subMeshes;
	std::vector<MeshPlacement> m_placements;
	std::string m_name;
	BoundingBox m_bounds;
};