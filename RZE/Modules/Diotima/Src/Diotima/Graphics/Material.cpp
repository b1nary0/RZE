#include <Diotima/Graphics/Material.h>

#include <Diotima/Driver/OpenGL.h>
#include <Diotima/Graphics/Texture2D.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/Platform/File.h>

namespace Diotima
{
	Diotima::GFXMaterial::GFXMaterial()
	{
	}

	Diotima::GFXMaterial::~GFXMaterial()
	{
	}

	void GFXMaterial::AddTexture(GFXTexture2D* texture)
	{
		if (texture->GetTextureType() == ETextureType::Diffuse)
		{
			mDiffuseTextures.emplace_back(texture);
		}
		else
		{
			mSpecularTextures.emplace_back(texture);
		}
	}

	bool GFXMaterial::IsTextured() const
	{
		return !mDiffuseTextures.empty() || !mSpecularTextures.empty();
	}
}
