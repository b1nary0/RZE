#pragma once

#include <EngineCore/Resources/ResourceHandler.h>

#include <Graphics/MaterialOverrides.h>
#include <Graphics/MaterialValues.h>
#include <Graphics/StaticMesh.h>

#include <memory>
#include <string>

class MaterialInstance;

// One of a mesh's material slots (StaticMesh::Slot) as one object draws it: every sub-mesh using the slot draws with
// GetMaterial(). That's its base material (the mesh's shared one, or a material asset the overrides swap in), or,
// while the object overrides some of its values, the object's own material: the base's values with the overrides on
// top.
class MaterialSlot
{
public:
	explicit MaterialSlot(const StaticMesh::Slot& meshSlot);

	// The mesh slot's name: its material asset's file name, unique within the mesh
	const std::string& GetName() const { return m_name; }

	// What this object draws the slot's sub-meshes with
	const MaterialInstance& GetMaterial() const;

	// The mesh's own material for the slot, shared by every object drawing the mesh
	std::shared_ptr<const MaterialInstance> GetSharedMaterial() const { return m_sharedMaterial; }

	// The material asset the slot draws: the one the overrides swap in, or the mesh's own. Invalid for a material made
	// in code.
	const Filepath& GetMaterialPath() const;

	// What the slot draws with: the base material's values with the overrides on top
	MaterialValues GetValues() const;

	const MaterialOverrides& GetOverrides() const { return m_overrides; }
	// Replaces the overrides, and with them what the slot draws with. A swapped-in material is loaded through the
	// ResourceHandler (a cache hit once preloaded) and becomes the base; one that fails to load leaves the mesh's own,
	// though the override is kept. On top of the base: its own material while any values are overridden (made on the
	// first, updated in place after), the base itself once none are. A slot without a shared material (a mesh that's
	// never drawn, e.g. in tests) only keeps the overrides.
	void SetOverrides(const MaterialOverrides& overrides);

private:
	// The swapped-in material if it loaded, the shared one otherwise
	const MaterialInstance* GetBaseMaterial() const;
	// Loads the overrides' Material when it has changed
	void UpdateSwappedMaterial();

private:
	std::string m_name;
	std::shared_ptr<const MaterialInstance> m_sharedMaterial;
	Filepath m_sharedMaterialPath;
	MaterialOverrides m_overrides;

	// The overrides' Material the swapped-in material was last loaded for, so a failed one isn't tried again on every
	// change to the values; invalid without one
	Filepath m_swappedMaterialPath;
	// While it's loaded: the resource, so it stays loaded, and its material
	ResourceHandle m_swappedMaterialResource;
	std::shared_ptr<const MaterialInstance> m_swappedMaterial;

	// Only while values are overridden
	std::shared_ptr<MaterialInstance> m_ownMaterial;
};
