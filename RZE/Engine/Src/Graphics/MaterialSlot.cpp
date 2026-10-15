#include <StdAfx.h>
#include <Graphics/MaterialSlot.h>

MaterialSlot::MaterialSlot(const std::string& name, const std::shared_ptr<const MaterialInstance>& sharedMaterial)
	: m_name(name)
	, m_sharedMaterial(sharedMaterial)
{
}
