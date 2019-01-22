#pragma once

#include <string>

#include <Utils/PrimitiveDefs.h>
#include <Utils/Math/Vector2D.h>

namespace Diotima
{
	namespace ETextureType
	{
		enum T : U32
		{
			Diffuse,
			Specular,
			Normal
		};
	}

	class GFXTexture2D
	{
	public:
		GFXTexture2D(ETextureType::T textureType = ETextureType::Diffuse);
		~GFXTexture2D();

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
