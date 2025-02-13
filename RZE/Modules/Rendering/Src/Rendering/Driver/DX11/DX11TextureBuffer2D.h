#pragma once

#include <Rendering/Driver/GFXBuffer.h>

struct ID3D11SamplerState;
struct ID3D11Texture2D;
struct ID3D11ShaderResourceView;
struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;

namespace Rendering
{
	class DX11Device;

	class DX11TextureBuffer2D : public ITextureBuffer2D
	{
	public:
		DX11TextureBuffer2D() {};
		~DX11TextureBuffer2D();

	// ITextureBuffer2D interface
	public:
		void Allocate(const void* data, const GFXTextureBufferParams& params) override;
		void Release() override;
		void SetActive(U32 textureSlot) override;
		void* GetTextureData() override;
		bool IsValid() override;

	public:
		void SetDevice(DX11Device* device);
		ID3D11ShaderResourceView& GetResourceView();
		ID3D11RenderTargetView& GetTargetView();
		ID3D11DepthStencilView& GetDepthView();
		ID3D11SamplerState& GetSamplerState();

		// #TODO
		// This is probably causing an API naming consistency apocolypse but I think
		// This Is The Way.
		ID3D11Texture2D* GetHWResource();

	private:
		DX11Device* m_device = nullptr;

		ID3D11Texture2D* m_resource = nullptr;
		ID3D11SamplerState* m_samplerState = nullptr;

		ID3D11ShaderResourceView* m_SRV = nullptr;
		ID3D11RenderTargetView* m_RTV = nullptr;
		ID3D11DepthStencilView* m_DSV = nullptr;

		bool bIsDepthTexture { false };
	};

}