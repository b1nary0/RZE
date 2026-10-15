#pragma once

#include <Graphics/MeshDraw.h>
#include <Graphics/MeshGeometry.h>

#include <Utils/Platform/Filepath.h>

#include <memory>
#include <string>
#include <vector>

class MaterialInstance;
class MaterialSlot;

// A mesh's sub-meshes, its material slots, and where each sub-mesh is placed. Never changed once created, so every
// object drawing it shares one (through a StaticMeshInstance) instead of copying it.
class StaticMesh
{
public:
	// One of the mesh's materials. Sub-meshes made with the same material (the same material asset, when loaded)
	// share a slot, so an object changes them all by changing the slot.
	struct Slot
	{
		// Its material asset's file name, unique within the mesh
		std::string Name;
		// Shared by every object drawing the mesh. Null only for a mesh that's never drawn (e.g. in tests).
		std::shared_ptr<const MaterialInstance> Material;
		// The material asset Material was made from; invalid for one made in code, or a sub-mesh without one
		Filepath MaterialPath;
	};

	struct SubMesh
	{
		MeshGeometry Geometry;
		// Index into GetSlots()
		U32 SlotIndex = 0;
	};

	// Places each sub-mesh once, at the mesh's origin
	static std::shared_ptr<const StaticMesh> Create(const std::string& name, std::vector<SubMesh>&& subMeshes, std::vector<Slot>&& slots);
	// For loaders: each placement draws one sub-mesh with its own transform; a sub-mesh can be placed more than once
	static std::shared_ptr<const StaticMesh> Create(const std::string& name, std::vector<SubMesh>&& subMeshes, std::vector<Slot>&& slots, std::vector<MeshPlacement>&& placements);

	const std::string& GetName() const { return m_name; }

	// Each sub-mesh once, however many times it's placed
	const std::vector<SubMesh>& GetSubMeshes() const { return m_subMeshes; }
	const std::vector<Slot>& GetSlots() const { return m_slots; }

	// Object-space box around every placed sub-mesh; a zero-sized box at the origin with none
	const BoundingBox& GetBounds() const { return m_bounds; }

	// Every draw this mesh makes, placed in the world by objectMatrices and drawn with materialSlots (one per
	// GetSlots() entry): one per placement, in placement order. outDraws points into this mesh, materialSlots and
	// objectMatrices, so they must outlive outDraws and stay where they are. Reuses outDraws' memory when the count
	// is unchanged.
	void BuildDraws(const MatrixMem& objectMatrices, const std::vector<MaterialSlot>& materialSlots, std::vector<MeshDraw>& outDraws) const;

private:
	StaticMesh(const std::string& name, std::vector<SubMesh>&& subMeshes, std::vector<Slot>&& slots, std::vector<MeshPlacement>&& placements);

private:
	std::string m_name;
	std::vector<SubMesh> m_subMeshes;
	std::vector<Slot> m_slots;
	std::vector<MeshPlacement> m_placements;
	// Indexed like m_placements: whether it's at the mesh's origin, so its draw shares the object's matrices.
	// Worked out once, rather than comparing matrices on every build.
	std::vector<bool> m_isAtOrigin;
	BoundingBox m_bounds;
};
