#include <StdAfx.h>
#include <Graphics/MaterialSlot.h>

#include <Graphics/Material.h>
#include <Graphics/MaterialResource.h>

MaterialSlot::MaterialSlot(const StaticMesh::Slot& meshSlot)
	: m_name(meshSlot.Name)
	, m_sharedMaterial(meshSlot.Material)
	, m_sharedMaterialPath(meshSlot.MaterialPath)
{
}

const MaterialInstance& MaterialSlot::GetMaterial() const
{
	return m_ownMaterial != nullptr ? *m_ownMaterial : *GetBaseMaterial();
}

const Filepath& MaterialSlot::GetMaterialPath() const
{
	return m_overrides.Material.has_value() ? *m_overrides.Material : m_sharedMaterialPath;
}

MaterialValues MaterialSlot::GetValues() const
{
	const MaterialInstance* const base = GetBaseMaterial();
	return m_overrides.ApplyTo(base != nullptr ? base->GetValues() : MaterialValues());
}

void MaterialSlot::SetOverrides(const MaterialOverrides& overrides)
{
	m_overrides = overrides;
	UpdateSwappedMaterial();

	const MaterialInstance* const base = GetBaseMaterial();
	if (!m_overrides.ChangesValues() || base == nullptr)
	{
		m_ownMaterial.reset();
		return;
	}

	// Named after its base, so it's made again when the base changes
	if (m_ownMaterial == nullptr || m_ownMaterial->GetName() != base->GetName())
	{
		m_ownMaterial = std::make_shared<MaterialInstance>(base->GetName());
	}
	m_ownMaterial->SetValues(GetValues());
}

const MaterialInstance* MaterialSlot::GetBaseMaterial() const
{
	return m_swappedMaterial != nullptr ? m_swappedMaterial.get() : m_sharedMaterial.get();
}

void MaterialSlot::UpdateSwappedMaterial()
{
	const Filepath requested = m_overrides.Material.value_or(Filepath());
	if (requested.GetRelativePath() == m_swappedMaterialPath.GetRelativePath())
	{
		return;
	}

	m_swappedMaterialPath = requested;
	m_swappedMaterial.reset();
	m_swappedMaterialResource = ResourceHandle::EmptyHandle();

	if (!requested.IsValid() || m_sharedMaterial == nullptr)
	{
		return;
	}

	ResourceHandler& resourceHandler = RZE().GetResourceHandler();
	ResourceHandle resource = resourceHandler.LoadResource<MaterialResource>(requested);
	const MaterialResource* const material = resource.IsValid() ? resourceHandler.GetResource<MaterialResource>(resource) : nullptr;
	if (material == nullptr || material->GetMaterial() == nullptr)
	{
		RZE_LOG_WARNING_ARGS("Material [%s] for slot [%s] didn't load; drawing the mesh's own.", requested.GetRelativePath().c_str(), m_name.c_str());
		return;
	}

	m_swappedMaterial = material->GetMaterial();
	m_swappedMaterialResource = std::move(resource);
}
