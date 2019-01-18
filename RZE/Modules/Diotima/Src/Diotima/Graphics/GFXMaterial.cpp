#include <Diotima/Graphics/GFXMaterial.h>

#include <Diotima/Driver/OpenGL/OpenGL.h>
#include <Diotima/Graphics/GFXTexture2D.h>

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
		switch (texture->GetTextureType())
		{
		case ETextureType::Diffuse:
		{
			mDiffuseTextures.emplace_back(texture);
		}
		break;

		case ETextureType::Specular:
		{
			mSpecularTextures.emplace_back(texture);
		}
		break;

		case ETextureType::Normal:
		{
			mNormalMaps.emplace_back(texture);
		}
		break;
		}
	}

	bool GFXMaterial::IsTextured() const
	{
		return !mDiffuseTextures.empty();
	}
}
