#pragma once

#include <Rendering/Driver/GFXBuffer.h>

#include <vector>

struct ID3D11SamplerState;
struct ID3D11Texture2D;
struct ID3D11ShaderResourceView;
struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;
struct ID3D11UnorderedAccessView;

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
		void SetActiveCS(U32 textureSlot) override;
		void* GetTextureData() override;
		bool IsValid() override;

	public:
		void SetDevice(DX11Device* device);
		ID3D11ShaderResourceView& GetResourceView();
		ID3D11RenderTargetView& GetTargetView();
		// Every slice of a texture array (e.g. for clearing); the texture itself for a single one
		ID3D11DepthStencilView& GetDepthView();
		// One slice, to render depth into it alone; 0 for a single texture
		ID3D11DepthStencilView& GetDepthSliceView(U32 slice);
		// Every slice of one mip, for compute shaders to write; only for textures made with bIsUnorderedAccess
		ID3D11UnorderedAccessView& GetUnorderedAccessView(U32 mip);
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
		// A depth texture array's per-slice views; empty for a single texture, whose m_DSV is its one slice
		std::vector<ID3D11DepthStencilView*> m_sliceDSVs;
		// One per mip, each covering every slice; empty unless made with bIsUnorderedAccess
		std::vector<ID3D11UnorderedAccessView*> m_mipUAVs;

		bool bIsDepthTexture { false };
	};

}