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

	// #TODO
	// This function is bad. Shouldn't need to directly access the device.
	// Should just ask the Renderer to allocate the necessary structure to represent
	// this render target.
	void RenderTargetTexture::Initialize(DX11GFXDevice* device)
	{
		GFXTextureBufferParams depthTextureParams = { 0 };
		depthTextureParams.bIsDepthTexture = true;
		depthTextureParams.bIsShaderResource = true;
		depthTextureParams.Height = mHeight;
		depthTextureParams.Width = mWidth;
		depthTextureParams.MipLevels = 0;
		depthTextureParams.MostDetailedMip = 0;
		depthTextureParams.SampleCount = 1;
		depthTextureParams.SampleQuality = 0;

		mTextureID = device->CreateRenderTarget2D(mWidth, mHeight);
		mDepthTextureID = device->CreateTextureBuffer2D(nullptr, depthTextureParams);

		mGFXTexture = device->GetTextureBuffer2D(mTextureID);
		mDepthTexture = device->GetTextureBuffer2D(mDepthTextureID);
	}

	U32 RenderTargetTexture::GetTextureID() const
	{
		return mTextureID;
	}

	U32 RenderTargetTexture::GetDepthTextureID() const
	{
		return mDepthTextureID;
	}

	DX11GFXTextureBuffer2D& RenderTargetTexture::GetGFXTexture()
	{
		AssertNotNull(mGFXTexture);
		return *mGFXTexture;
	}

	DX11GFXTextureBuffer2D& RenderTargetTexture::GetDepthTexture()
	{
		AssertNotNull(mDepthTexture);
		return *mDepthTexture;
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