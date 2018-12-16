#pragma once

#include <string>

#include <Utils/PrimitiveDefs.h>
#include <Utils/Interfaces/Resource.h>
#include <Utils/Math/Vector2D.h>

namespace Diotima
{
	const FilePath kDefaultDiffuseTexturePath{ "Engine/Assets/2D/NOASSET_Texture.png" };

	namespace ETextureType
	{
		enum T : U32
		{
			Diffuse,
			Specular,
			Normal
		};
	}

	class GFXTexture2D : public IResource
	{
	public:
		GFXTexture2D(ETextureType::T textureType = ETextureType::Diffuse);
		~GFXTexture2D();

		virtual bool Load(const FilePath& filePath) override;
		virtual void Release() override;

		U32				GetTextureID();
		ETextureType::T	GetTextureType();
		Vector2D		GetDimensions();

	private:
		U32 mTextureID;

		ETextureType::T mTextureType;

		Int32 mWidth;
		Int32 mHeight;
		Int32 mChannels;
	};
}
