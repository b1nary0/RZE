#pragma once

#include <Diotima/Driver/GFXBuffer.h>

struct ID3D11SamplerState;
struct ID3D11Texture2D;
struct ID3D11ShaderResourceView;

namespace Diotima
{
	class DX11GFXDevice;

	class DX11GFXTextureBuffer2D : public IGFXTextureBuffer2D
	{
	public:
		DX11GFXTextureBuffer2D() = default;
		~DX11GFXTextureBuffer2D() = default;

	// IGFXTextureBuffer2D interface
	public:
		virtual void Allocate(void* data, U32 width, U32 height) override;

	public:
		void SetDevice(DX11GFXDevice* device);
		ID3D11ShaderResourceView& GetResourceView();
		ID3D11SamplerState& GetSamplerState();

	private:
		DX11GFXDevice* mDevice;

		ID3D11Texture2D* mResource;
		ID3D11ShaderResourceView* mResourceView;
		ID3D11SamplerState* mSamplerState;
	};

}