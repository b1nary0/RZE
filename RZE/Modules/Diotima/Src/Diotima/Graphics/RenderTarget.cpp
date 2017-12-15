#include <Diotima/Graphics/RenderTarget.h>

namespace Diotima
{
	
	RenderTarget::RenderTarget()
		: mFrameBufferID(0)
		, mDepthBufferID(0)
		, mColorBufferID(0)
		, mWidth(1024)
		, mHeight(1024)
	{
	}

	RenderTarget::RenderTarget(U32 width, U32 height)
		: mWidth(width)
		, mHeight(height)
	{
	}

	RenderTarget::~RenderTarget()
	{
	}

	RenderTargetTexture::RenderTargetTexture(U32 width, U32 height)
		: RenderTarget(width, height)
	{
	}

}