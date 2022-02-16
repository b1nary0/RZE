#pragma once

#include <Rendering/BufferHandle.h>

#include <Utils/PrimitiveDefs.h>

namespace Rendering
{
	class RenderTargetTexture
	{
	public:
		RenderTargetTexture() = delete;
		RenderTargetTexture(U32 width, U32 height);
		virtual ~RenderTargetTexture();

	public:
		void Initialize();
		
		RenderTargetHandle GetTargetPlatformObject() const;
		TextureBuffer2DHandle GetDepthTexturePlatformObject() const;

		U32 GetWidth() const;
		U32 GetHeight() const;

	private:
		U32 m_width;
		U32 m_height;

	private:
		RenderTargetHandle m_target;
		TextureBuffer2DHandle m_depthTexture;
	};
}
