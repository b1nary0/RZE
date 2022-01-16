#pragma once

#include <Utils/Math/Vector2D.h>
#include <Utils/PrimitiveDefs.h>

struct ID3D11DepthStencilView;
struct ID3D11RenderTargetView;

namespace Rendering
{
	class DX11GFXDevice;
	class DX11GFXTextureBuffer2D;

	class RenderTargetTexture
	{
	public:
		RenderTargetTexture() = delete;
		RenderTargetTexture(U32 width, U32 height);
		virtual ~RenderTargetTexture() = default;

	public:
		void Initialize(DX11GFXDevice* device);

		U32 GetTextureID() const;
		U32 GetDepthTextureID() const;
		DX11GFXTextureBuffer2D& GetGFXTexture();
		DX11GFXTextureBuffer2D& GetDepthTexture();

		U32 GetWidth() const;
		U32 GetHeight() const;

	private:
		U32 mWidth;
		U32 mHeight;

	private:
		U32 mTextureID;
		U32 mDepthTextureID;

		// All these driver-specific classes will need
		// to be moved with new API.
		DX11GFXTextureBuffer2D* mGFXTexture;
		DX11GFXTextureBuffer2D* mDepthTexture;
	};
}
