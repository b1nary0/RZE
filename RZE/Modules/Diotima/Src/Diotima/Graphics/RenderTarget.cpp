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


	bool RenderTarget::SetDimensions(U32 width, U32 height)
	{
		if (width != mWidth || height != mHeight)
		{
			mWidth = width;
			mHeight = height;

			return true;
		}

		return false;
	}

	RenderTargetTexture::RenderTargetTexture(U32 width, U32 height)
		: RenderTarget(width, height)
	{
	}

}