#pragma once

#include <Utils/PrimitiveDefs.h>

#include <Diotima/Driver/DX11/DX11GFXTextureBuffer2D.h>

#include <memory>
#include <vector>

namespace Diotima
{
	class DX11GFXDevice;
	class DX11GFXTextureBuffer2D;

	// #TODO
	// Prototypical for TexturePack first implementation
	struct TextureData
	{
		int mWidth;
		int mHeight;
		U8* mData;
	};

	// Describes what is essentially a TextureArray
	class TexturePack
	{
	public:
		TexturePack() = default;
		~TexturePack() = default;

	public:
		bool Allocate(DX11GFXDevice& hwDevice, const std::vector<TextureData>& textureData);

		size_t GetResourceCount() const;
		DX11GFXTextureBuffer2D* GetResourceAt(size_t index);

	private:
		std::vector<std::unique_ptr<DX11GFXTextureBuffer2D>> mResources;
	};

	inline size_t TexturePack::GetResourceCount() const
	{
		return mResources.size();
	}
}