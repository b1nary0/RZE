#include <StdAfx.h>
#include <Graphics/Material.h>

#include <Graphics/Texture2D.h>

MaterialInstance::MaterialInstance()
{
	m_textureSlots.resize(TEXTURE_SLOT_COUNT);
}

MaterialInstance::~MaterialInstance()
{
}

void MaterialInstance::SetTexture(U8 textureSlot, const ResourceHandle& textureResource)
{
	AssertExpr(textureSlot < TEXTURE_SLOT_COUNT);
	m_textureSlots[textureSlot] = textureResource;
}

const ResourceHandle& MaterialInstance::GetTexture(U8 textureSlot) const
{
	AssertExpr(textureSlot < TEXTURE_SLOT_COUNT);
	return m_textureSlots[textureSlot];
}

ResourceHandle MaterialInstance::GetTexture(U8 textureSlot)
{
	AssertExpr(textureSlot < TEXTURE_SLOT_COUNT);
	return m_textureSlots[textureSlot];
}

void MaterialInstance::SetShaderTechnique(const ResourceHandle& shaderTechnique)
{
	m_shaderTechnique = shaderTechnique;
}
