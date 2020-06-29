#pragma once

#include <Diotima/Driver/GFXBuffer.h>

struct ID3D11SamplerState;
struct ID3D11Texture2D;
struct ID3D11ShaderResourceView;
struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;

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
		virtual void Allocate(void* data, const GFXTextureBufferParams& params) override;
		virtual void Release() override;

	public:
		// #TODO(Hack asf)
		void SetIsRenderTarget();

	public:
		void SetDevice(DX11GFXDevice* device);
		ID3D11ShaderResourceView& GetResourceView();
		ID3D11RenderTargetView& GetTargetView();
		ID3D11DepthStencilView& GetDepthView();
		ID3D11SamplerState& GetSamplerState();

	private:
		DX11GFXDevice* mDevice;

		ID3D11Texture2D* mResource;
		ID3D11SamplerState* mSamplerState;

		ID3D11ShaderResourceView* mSRV;
		ID3D11RenderTargetView* mRTV;
		ID3D11DepthStencilView* mDSV;

		bool bIsRenderTarget;
	};

}