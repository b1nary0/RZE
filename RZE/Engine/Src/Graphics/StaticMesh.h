#pragma once

#include <Graphics/MeshDraw.h>
#include <Graphics/MeshGeometry.h>

#include <memory>
#include <string>
#include <vector>

// A mesh's sub-meshes and where each is placed. Never changed once created, so every object drawing it shares one
// (through a StaticMeshInstance) instead of copying it.
class StaticMesh
{
public:
	// Places each sub-mesh once, at the mesh's origin
	static std::shared_ptr<const StaticMesh> Create(std::string name, std::vector<MeshGeometry> subMeshes);
	// For loaders: each placement draws one sub-mesh with its own transform; a sub-mesh can be placed more than once
	static std::shared_ptr<const StaticMesh> Create(std::string name, std::vector<MeshGeometry> subMeshes, std::vector<MeshPlacement> placements);

	const std::string& GetName() const { return m_name; }

	// Each sub-mesh's geometry and material, once however many times it's placed
	const std::vector<MeshGeometry>& GetSubMeshes() const { return m_subMeshes; }

	// Object-space box around every placed sub-mesh; a zero-sized box at the origin with none
	const BoundingBox& GetBounds() const { return m_bounds; }

	// Every draw this mesh makes, placed in the world by objectMatrices: one per placement, in placement order.
	// outDraws' geometry points into this mesh, and a draw at the mesh's origin points at objectMatrices, so both must
	// outlive outDraws and stay where they are. Reuses outDraws' memory when the count is unchanged.
	void BuildDraws(const MatrixMem& objectMatrices, std::vector<MeshDraw>& outDraws) const;

private:
	StaticMesh(std::string name, std::vector<MeshGeometry> subMeshes, std::vector<MeshPlacement> placements);

private:
	std::string m_name;
	std::vector<MeshGeometry> m_subMeshes;
	std::vector<MeshPlacement> m_placements;
	// Indexed like m_placements: whether it's at the mesh's origin, so its draw shares the object's matrices.
	// Worked out once, rather than comparing matrices on every build.
	std::vector<bool> m_isAtOrigin;
	BoundingBox m_bounds;
};
