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

		virtual void Initialize();
		virtual void Bind() override;
		virtual void Unbind() override;

	private:

	};
}