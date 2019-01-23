#include <StdAfx.h>
#include <Graphics/Material.h>

#include <Graphics/Texture2D.h>

Material::Material()
	: mDiffuseMap(nullptr)
	, mNormalMap(nullptr)
	, mSpecularMap(nullptr)
{
}

Material::~Material()
{
}

void Material::SetDiffuse(Texture2D* texture)
{
	AssertExpr(texture != nullptr && texture->GetTextureType() == ETextureType::Diffuse);
	mDiffuseMap = texture;
}

void Material::SetSpecular(Texture2D* texture)
{
	AssertExpr(texture != nullptr && texture->GetTextureType() == ETextureType::Specular);
	mSpecularMap = texture;
}

void Material::SetNormal(Texture2D* texture)
{
	AssertExpr(texture != nullptr && texture->GetTextureType() == ETextureType::Normal);
	mNormalMap = texture;
}

const Texture2D& Material::GetDiffuse() const
{
	AssertNotNull(mDiffuseMap);
	return *mDiffuseMap;
}

const Texture2D& Material::GetSpecular() const
{
	AssertNotNull(mSpecularMap);
	return *mSpecularMap;
}

const Texture2D& Material::GetNormal() const
{
	AssertNotNull(mNormalMap);
	return *mNormalMap;
}

bool Material::HasDiffuse() const
{
	return mDiffuseMap != nullptr;
}

bool Material::HasSpecular() const
{
	return mSpecularMap != nullptr;
}

bool Material::HasNormal() const
{
	return mNormalMap != nullptr;
}

bool Material::IsTextured() const
{
	// #TODO(Josh::This needs to be a more concise evaluation)
	return mDiffuseMap != nullptr;
}
