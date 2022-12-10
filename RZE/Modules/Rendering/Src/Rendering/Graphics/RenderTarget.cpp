#include <Rendering/Graphics/RenderTarget.h>

#include <Rendering/Renderer.h>
#include <Rendering/Driver/GFXBuffer.h>

namespace Rendering
{

	RenderTargetTexture::RenderTargetTexture(U32 width, U32 height)
		: m_width(width)
		, m_height(height)
	{
	}

	RenderTargetTexture::~RenderTargetTexture()
	{
	}
	
	void RenderTargetTexture::Initialize()
	{
		GFXTextureBufferParams depthTextureParams = { 0 };
		depthTextureParams.bIsDepthTexture = true;
		depthTextureParams.bIsShaderResource = true;
		depthTextureParams.Height = m_height;
		depthTextureParams.Width = m_width;
		depthTextureParams.MipLevels = 0;
		depthTextureParams.MostDetailedMip = 0;
		depthTextureParams.SampleCount = 1;
		depthTextureParams.SampleQuality = 0;

		m_depthTexture = Rendering::Renderer::CreateTextureBuffer2D(nullptr, depthTextureParams);

		GFXTextureBufferParams targetParams = { 0 };
		targetParams.bIsRenderTarget = true;
		targetParams.bIsShaderResource = true;
		targetParams.Height = m_height;
		targetParams.Width = m_width;
		targetParams.MipLevels = 0;
		targetParams.MostDetailedMip = 0;
		targetParams.SampleCount = 1;
		targetParams.SampleQuality = 0;
		
		m_target = Rendering::Renderer::CreateTextureBuffer2D(nullptr, targetParams);
	}
	
	RenderTargetHandle RenderTargetTexture::GetTargetPlatformObject() const
	{
		return m_target;
	}

	TextureBuffer2DHandle RenderTargetTexture::GetDepthTexturePlatformObject() const
	{
		return m_depthTexture;
	}

	U32 RenderTargetTexture::GetWidth() const
	{
		return m_width;
	}

	U32 RenderTargetTexture::GetHeight() const
	{
		return m_height;
	}

}