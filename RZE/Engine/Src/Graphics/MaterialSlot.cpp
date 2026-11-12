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

GraphicsLayouts::InstanceMaterialLayout MaterialSlot::BuildInstanceData() const
{
	// GetValues' answer for just these, without its copy of the texture paths: this is asked for every draw, every
	// pass. An own material already has the overrides, which then replace its values with the same ones.
	static const MaterialValues k_defaultValues;
	const MaterialInstance* const material = m_ownMaterial != nullptr ? m_ownMaterial.get() : GetBaseMaterial();
	const MaterialValues& values = material != nullptr ? material->GetValues() : k_defaultValues;
	return MaterialInstance::BuildInstanceData(
		m_overrides.Tint.has_value() ? *m_overrides.Tint : values.Tint,
		m_overrides.Shininess.value_or(values.Shininess),
		m_overrides.Opacity.value_or(values.Opacity),
		m_overrides.BlendMode.value_or(values.BlendMode));
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
	if (!m_overrides.ChangesBindings() || base == nullptr)
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
	if (requested == m_swappedMaterialPath)
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
