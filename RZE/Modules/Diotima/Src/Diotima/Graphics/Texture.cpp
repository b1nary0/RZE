#include <Diotima/Graphics/Texture.h>

#include <Diotima/Driver/DX11/DX11GFXDevice.h>

#include <Utils/DebugUtils/Debug.h>

namespace Diotima
{
	bool TexturePack::Allocate(DX11GFXDevice& hwDevice, const std::vector<TextureData>& textureData)
	{
		mResources.reserve(textureData.size());

		for (size_t index = 0; index < textureData.size(); ++index)
		{
			GFXTextureBufferParams params = { 0 };
			params.bIsRenderTarget = true;
			params.bIsShaderResource = true;
			params.Height = textureData[index].mHeight;
			params.Width = textureData[index].mWidth;
			params.MipLevels = 0;
			params.MostDetailedMip = 0;
			params.SampleCount = 1;
			params.SampleQuality = 0;

			mResources.push_back(std::make_unique<DX11GFXTextureBuffer2D>());
			mResources.back()->SetDevice(&hwDevice);
			mResources.back()->Allocate(textureData[index].mData, params);
		}

		return true;
	}

	Diotima::DX11GFXTextureBuffer2D* TexturePack::GetResourceAt(size_t index)
	{
		AssertExpr(index < mResources.size());
		return mResources[index].get();
	}

}