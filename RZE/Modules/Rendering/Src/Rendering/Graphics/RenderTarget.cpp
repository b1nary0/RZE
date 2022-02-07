#include <Rendering/Graphics/RenderTarget.h>

#include <Rendering/Driver/DX11/DX11Device.h>
#include <Rendering/Driver/DX11/DX11TextureBuffer2D.h>

#include <Utils/DebugUtils/Debug.h>

namespace Rendering
{

	RenderTargetTexture::RenderTargetTexture(U32 width, U32 height)
		: mWidth(width)
		, mHeight(height)
	{
	}

	RenderTargetTexture::~RenderTargetTexture()
	{
	}

	// #TODO
	// This function is bad. Shouldn't need to directly access the device.
	// Should just ask the Renderer to allocate the necessary structure to represent
	// this render target.
	void RenderTargetTexture::Initialize(DX11Device* device)
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

		//mTextureID = device->CreateRenderTarget2D(mWidth, mHeight);
		//mDepthTextureID = device->CreateTextureBuffer2D(nullptr, depthTextureParams);

		//mGFXTexture = device->GetTextureBuffer2D(mTextureID);
		//mDepthTexture = device->GetTextureBuffer2D(mDepthTextureID);
	}

	U32 RenderTargetTexture::GetTextureID() const
	{
		return mTextureID;
	}

	U32 RenderTargetTexture::GetDepthTextureID() const
	{
		return mDepthTextureID;
	}

	DX11TextureBuffer2D& RenderTargetTexture::GetGFXTexture()
	{
		AssertNotNull(mGFXTexture);
		return *mGFXTexture;
	}

	DX11TextureBuffer2D& RenderTargetTexture::GetDepthTexture()
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