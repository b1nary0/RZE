#pragma once

#include <memory>
#include <string>

class MaterialInstance;

// One of a mesh's material slots (StaticMesh::Slot) as one object draws it: every sub-mesh using the slot draws with
// GetMaterial(). For now that's always the mesh's shared material; per-object overrides come with
// StaticMeshInstance's material overrides.
class MaterialSlot
{
public:
	MaterialSlot(const std::string& name, const std::shared_ptr<const MaterialInstance>& sharedMaterial);

	// The mesh slot's name: its material asset's file name, unique within the mesh
	const std::string& GetName() const { return m_name; }

	// What this object draws the slot's sub-meshes with
	const MaterialInstance& GetMaterial() const { return *m_sharedMaterial; }

	// The mesh's own material for the slot, shared by every object drawing the mesh
	std::shared_ptr<const MaterialInstance> GetSharedMaterial() const { return m_sharedMaterial; }

private:
	std::string m_name;
	std::shared_ptr<const MaterialInstance> m_sharedMaterial;
};
