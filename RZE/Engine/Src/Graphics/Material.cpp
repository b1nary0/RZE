#include <StdAfx.h>
#include <Graphics/Material.h>

#include <Graphics/Texture2D.h>

Material::Material()
{
	m_textureSlots.resize(TEXTURE_SLOT_COUNT);
}

Material::~Material()
{
}

void Material::SetTexture(U8 textureSlot, const ResourceHandle& textureResource)
{
	AssertExpr(textureSlot < TEXTURE_SLOT_COUNT);
	m_textureSlots[textureSlot] = textureResource;
}

const ResourceHandle& Material::GetTexture(U8 textureSlot) const
{
	AssertExpr(textureSlot < TEXTURE_SLOT_COUNT);
	return m_textureSlots[textureSlot];
}

ResourceHandle Material::GetTexture(U8 textureSlot)
{
	AssertExpr(textureSlot < TEXTURE_SLOT_COUNT);
	return m_textureSlots[textureSlot];
}

void Material::SetShaderTechnique(const ResourceHandle& shaderTechnique)
{
	m_shaderTechnique = shaderTechnique;
}
