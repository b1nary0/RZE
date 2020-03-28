#pragma once

#include <Utils/Math/Vector2D.h>
#include <Utils/PrimitiveDefs.h>

struct ID3D11Texture2D;
struct ID3D11RenderTargetView;


namespace Diotima
{
	class DX11GFXDevice;

	class RenderTargetTexture
	{
	public:
		RenderTargetTexture() = delete;
		RenderTargetTexture(U32 width, U32 height);
		virtual ~RenderTargetTexture() = default;

	public:
		void Initialize(DX11GFXDevice* device);

		U32 GetTextureID() const;

	private:
		U32 mWidth;
		U32 mHeight;

	private:
		U32 mTextureID;
	};
}
