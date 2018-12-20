#pragma once

#include <Diotima/Graphics/RenderTarget.h>

namespace Diotima
{
	class GLRenderTargetTexture : public RenderTargetTexture
	{
	public:
		GLRenderTargetTexture();
		virtual ~GLRenderTargetTexture();

	public:
		virtual void Initialize() override;
		virtual void Bind() override;
		virtual void Unbind() override;
	};

	class GLRenderTargetTextureMSAA : public RenderTargetTexture
	{
	public:
		GLRenderTargetTextureMSAA();
		virtual ~GLRenderTargetTextureMSAA();

	public:
		virtual void Initialize() override;
		virtual void Bind() override;
		virtual void Unbind() override;
	};

	class GLRenderTargetDepthTexture : public GLRenderTargetTexture
	{
	public:
		GLRenderTargetDepthTexture();
		virtual ~GLRenderTargetDepthTexture();

	public:
		virtual void Initialize() override;
	};
}