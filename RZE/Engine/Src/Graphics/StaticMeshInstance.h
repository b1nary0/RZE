#pragma once

#include <Graphics/MeshDraw.h>
#include <Graphics/MeshGeometry.h>

class StaticMeshInstance
{
public:
	StaticMeshInstance();
	~StaticMeshInstance();

	// Places each sub-mesh once, at the mesh's origin
	void Initialize(const std::vector<MeshGeometry>& meshGeometry);
	// For loaders: each placement draws one sub-mesh with its own transform; a sub-mesh can be placed more than once
	void Initialize(const std::vector<MeshGeometry>& meshGeometry, std::vector<MeshPlacement>&& placements);

	// Each sub-mesh's geometry and material, once however many times it's placed
	std::vector<MeshGeometry>& GetSubMeshes();
	const std::vector<MeshGeometry>& GetSubMeshes() const;

	const std::string& GetName() const;
	void SetName(const std::string& name);

	// Object-space box around every placed sub-mesh, calculated in Initialize(); a zero-sized box at the origin with none
	const BoundingBox& GetBounds() const { return m_bounds; }

	// Every draw this mesh makes, placed in the world by objectMatrices: one per placement, in placement order.
	// outDraws' geometry points into this mesh, and a draw at the mesh's origin points at objectMatrices, so both must
	// outlive outDraws and stay where they are. Reuses outDraws' memory when the count is unchanged.
	void BuildDraws(const MatrixMem& objectMatrices, std::vector<MeshDraw>& outDraws) const;

private:
	void CalculateBounds();

private:
	std::vector<MeshGeometry> m_subMeshes;
	std::vector<MeshPlacement> m_placements;
	// Indexed like m_placements: whether it's at the mesh's origin, so its draw shares the object's matrices.
	// Worked out once in Initialize rather than comparing matrices on every build.
	std::vector<bool> m_isAtOrigin;
	std::string m_name;
	BoundingBox m_bounds;
};
