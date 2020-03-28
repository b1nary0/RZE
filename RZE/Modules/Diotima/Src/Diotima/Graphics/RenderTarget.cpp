#include <Diotima/Graphics/RenderTarget.h>

#include <Diotima/Driver/DX11/DX11GFXDevice.h>
#include <Diotima/Driver/DX11/DX11GFXTextureBuffer2D.h>

#include <Utils/DebugUtils/Debug.h>

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
		mGFXTexture = device->GetTextureBuffer2D(mTextureID);
	}

	U32 RenderTargetTexture::GetTextureID() const
	{
		return mTextureID;
	}

	DX11GFXTextureBuffer2D& RenderTargetTexture::GetGFXTexture()
	{
		AssertNotNull(mGFXTexture);
		return *mGFXTexture;
	}

	U32 RenderTargetTexture::GetWidth() const
	{
		return mWidth;
	}

	U32 RenderTargetTexture::GetHeight() const
	{
		return mHeight;
	}

}