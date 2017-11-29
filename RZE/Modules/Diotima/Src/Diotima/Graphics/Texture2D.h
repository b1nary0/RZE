#pragma once

#include <string>

#include <Utils/Interfaces/Resource.h>
#include <Utils/PrimitiveDefs.h>

namespace Diotima
{
	class GFXTexture2D : public IResource
	{
	public:
		GFXTexture2D();
		~GFXTexture2D();

		virtual bool Load(const std::string& filePath) override;

		U32 GetTextureID();

		Vector2D GetDimensions();

	private:
		U32 mTextureID;

		Int32 mWidth;
		Int32 mHeight;
		Int32 mChannels;
	};
}
