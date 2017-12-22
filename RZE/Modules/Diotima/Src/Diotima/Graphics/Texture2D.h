#pragma once

#include <string>

#include <Utils/PrimitiveDefs.h>
#include <Utils/Interfaces/Resource.h>
#include <Utils/Math/Vector2D.h>

namespace Diotima
{
	namespace ETextureType
	{
		enum T
		{
			Diffuse,
			Specular
		};
	}

	class GFXTexture2D : public IResource
	{
	public:
		GFXTexture2D(ETextureType::T textureType = ETextureType::Diffuse);
		~GFXTexture2D();

		virtual bool Load(const std::string& filePath) override;

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
