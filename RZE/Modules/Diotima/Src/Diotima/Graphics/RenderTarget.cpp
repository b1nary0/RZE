#include <Diotima/Graphics/RenderTarget.h>

#include <Diotima/Driver/DX11/DX11GFXDevice.h>

namespace Diotima
{

	RenderTargetTexture::RenderTargetTexture(U32 width, U32 height)
		: mWidth(width)
		, mHeight(height)
	{
	}

	void RenderTargetTexture::Initialize(DX11GFXDevice* device)
	{
		mTextureID = device->CreateRenderTarget2D(mWidth, mHeight);
	}

	U32 RenderTargetTexture::GetTextureID() const
	{
		return mTextureID;
	}

}