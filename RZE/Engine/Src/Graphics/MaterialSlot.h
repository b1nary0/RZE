#pragma once

#include <Graphics/MaterialOverrides.h>
#include <Graphics/MaterialValues.h>

#include <memory>
#include <string>

class MaterialInstance;

// One of a mesh's material slots (StaticMesh::Slot) as one object draws it: every sub-mesh using the slot draws with
// GetMaterial(). That's the mesh's shared material, or, while the object overrides some of its values, the object's
// own material: the shared values with the overrides on top.
class MaterialSlot
{
public:
	MaterialSlot(const std::string& name, const std::shared_ptr<const MaterialInstance>& sharedMaterial);

	// The mesh slot's name: its material asset's file name, unique within the mesh
	const std::string& GetName() const { return m_name; }

	// What this object draws the slot's sub-meshes with
	const MaterialInstance& GetMaterial() const;

	// The mesh's own material for the slot, shared by every object drawing the mesh
	std::shared_ptr<const MaterialInstance> GetSharedMaterial() const { return m_sharedMaterial; }

	// What the slot draws with: the shared material's values with the overrides on top
	MaterialValues GetValues() const;

	const MaterialOverrides& GetOverrides() const { return m_overrides; }
	// Replaces the overrides, and with them what the slot draws with: its own material while there are any (made on
	// the first, updated in place after), the shared one once they're all gone. A slot without a shared material (a
	// mesh that's never drawn, e.g. in tests) only keeps the overrides.
	void SetOverrides(const MaterialOverrides& overrides);

private:
	std::string m_name;
	std::shared_ptr<const MaterialInstance> m_sharedMaterial;
	MaterialOverrides m_overrides;
	// Only while there are overrides
	std::shared_ptr<MaterialInstance> m_ownMaterial;
};
