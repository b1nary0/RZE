#pragma once

#include <Utils/PrimitiveDefs.h>

#include <Diotima/Driver/DX11/DX11GFXTextureBuffer2D.h>

#include <memory>
#include <vector>

struct ID3D11ShaderResourceView;

namespace Diotima
{
	class DX11GFXDevice;
	class DX11GFXTextureBuffer2D;

	// #TODO
	// Prototypical for TexturePack first implementation
	struct TextureData
	{
		struct TextureDataHeader
		{
			int mWidth;
			int mHeight;
		};

		TextureDataHeader mHeader;
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
		void Release();

		size_t GetResourceCount() const;
		DX11GFXTextureBuffer2D* GetResourceAt(size_t index);

		std::vector<ID3D11ShaderResourceView*> GetAsGPUTextureArray() const;

	private:
		std::vector<std::unique_ptr<DX11GFXTextureBuffer2D>> mResources;
	};

	inline size_t TexturePack::GetResourceCount() const
	{
		return mResources.size();
	}
}