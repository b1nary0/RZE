#include <StdAfx.h>
#include <Graphics/Material.h>

#include <Graphics/Texture2D.h>

Material::Material()
{
	mTextureSlots.resize(TEXTURE_SLOT_COUNT);
}

Material::~Material()
{
}

void Material::SetTexture(U8 textureSlot, const ResourceHandle& textureResource)
{
	AssertExpr(textureSlot < TEXTURE_SLOT_COUNT);
	mTextureSlots[textureSlot] = textureResource;
}

const ResourceHandle& Material::GetTexture(U8 textureSlot) const
{
	AssertExpr(textureSlot < TEXTURE_SLOT_COUNT);
	return mTextureSlots[textureSlot];
}

ResourceHandle Material::GetTexture(U8 textureSlot)
{
	AssertExpr(textureSlot < TEXTURE_SLOT_COUNT);
	return mTextureSlots[textureSlot];
}

void Material::SetShaderTechnique(const ResourceHandle& shaderTechnique)
{
	mShaderTechnique = shaderTechnique;
}
