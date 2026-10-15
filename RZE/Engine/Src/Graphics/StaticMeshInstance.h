#pragma once

#include <Graphics/MaterialSlot.h>
#include <Graphics/StaticMesh.h>

#include <EngineCore/Resources/ResourceHandler.h>

#include <memory>
#include <vector>

// A StaticMesh as one object draws it. The mesh is shared; what's per object lives here (its material slots). Its
// mesh never changes: swapping meshes means a new instance, handed to RenderComponent::SetMeshInstance. An instance
// belongs to one object.
class StaticMeshInstance
{
public:
	// meshResource, if given, is the loaded asset mesh came from (see StaticMeshResource::CreateInstance): holding it
	// keeps the asset loaded while the instance lives, and gives its path. Empty for a mesh made in code.
	static std::shared_ptr<StaticMeshInstance> Create(const std::shared_ptr<const StaticMesh>& mesh, ResourceHandle&& meshResource = ResourceHandle::EmptyHandle());

	const StaticMesh& GetMesh() const { return *m_mesh; }

	// The mesh asset's path; empty for a mesh made in code
	const Filepath& GetResourcePath() const;

	// One per mesh slot (StaticMesh::GetSlots), in the same order. Never resized, so draws can point at them.
	std::vector<MaterialSlot>& GetMaterialSlots() { return m_materialSlots; }
	const std::vector<MaterialSlot>& GetMaterialSlots() const { return m_materialSlots; }

	// Replaces every slot's overrides with overridesByName's for its name (none for a slot it doesn't name). Ones for
	// names this mesh has no slot for are kept too, so GetOverrides still has them: swapping to another mesh and back,
	// or saving, doesn't lose them.
	void SetOverrides(const MaterialOverridesByName& overridesByName);
	// Each slot's overrides by its name, for the slots that have any, and the ones kept for slots this mesh hasn't got
	MaterialOverridesByName GetOverrides() const;

	// See StaticMesh::BuildDraws; the draws use this instance's material slots
	void BuildDraws(const MatrixMem& objectMatrices, std::vector<MeshDraw>& outDraws) const;

private:
	StaticMeshInstance(const std::shared_ptr<const StaticMesh>& mesh, ResourceHandle&& meshResource);

private:
	// Held directly so drawing and bounds reach the mesh without a ResourceHandler table lookup. The handle alone
	// would be enough to find it, but it would have to be resolved every time.
	std::shared_ptr<const StaticMesh> m_mesh;
	// Only for the resource's availability: holding it keeps the mesh asset loaded (and cached, so loading it again
	// is a hit) while this instance lives, and gives its path. Empty for a mesh made in code.
	ResourceHandle m_meshResource;
	std::vector<MaterialSlot> m_materialSlots;
	// Overrides SetOverrides was given for slot names this mesh hasn't got
	MaterialOverridesByName m_unmatchedOverrides;
};
