#include <StdAfx.h>
#include <Graphics/MaterialSlot.h>

#include <Graphics/Material.h>

MaterialSlot::MaterialSlot(const std::string& name, const std::shared_ptr<const MaterialInstance>& sharedMaterial)
	: m_name(name)
	, m_sharedMaterial(sharedMaterial)
{
}

const MaterialInstance& MaterialSlot::GetMaterial() const
{
	return m_ownMaterial != nullptr ? *m_ownMaterial : *m_sharedMaterial;
}

MaterialValues MaterialSlot::GetValues() const
{
	const MaterialValues base = m_sharedMaterial != nullptr ? m_sharedMaterial->GetValues() : MaterialValues();
	return m_overrides.ApplyTo(base);
}

void MaterialSlot::SetOverrides(const MaterialOverrides& overrides)
{
	m_overrides = overrides;

	if (m_overrides.IsEmpty() || m_sharedMaterial == nullptr)
	{
		m_ownMaterial.reset();
		return;
	}

	if (m_ownMaterial == nullptr)
	{
		m_ownMaterial = std::make_shared<MaterialInstance>(m_sharedMaterial->GetName());
	}
	m_ownMaterial->SetValues(GetValues());
}
