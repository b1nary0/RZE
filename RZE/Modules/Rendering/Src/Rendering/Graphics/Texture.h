#pragma once

#include <Utils/PrimitiveDefs.h>

#include <Rendering/Driver/DX11/DX11TextureBuffer2D.h>

#include <memory>
#include <vector>

struct ID3D11ShaderResourceView;

namespace Rendering
{
	class DX11Device;
	class DX11TextureBuffer2D;

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
		bool Allocate(DX11Device& hwDevice, const std::vector<TextureData>& textureData);
		void Release();

		size_t GetResourceCount() const;
		DX11TextureBuffer2D* GetResourceAt(size_t index);

		std::vector<ID3D11ShaderResourceView*> GetAsGPUTextureArray() const;

	private:
		std::vector<std::unique_ptr<DX11TextureBuffer2D>> mResources;
	};

	inline size_t TexturePack::GetResourceCount() const
	{
		return mResources.size();
	}
}