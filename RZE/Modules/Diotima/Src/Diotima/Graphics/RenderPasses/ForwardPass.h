#pragma once

#include <Diotima/Graphics/RenderPass.h>

#include <Diotima/Renderer.h>

#include <Diotima/Driver/DX12/DX12.h>

namespace Diotima
{
	class DX12GFXDevice;
	class DX12GFXDepthStencilBuffer;

	class ForwardPass : public GFXRenderPass
	{
	public:
		// #TODO(device parameter should eventually be a context (cmd list of its own)
		ForwardPass();
		virtual ~ForwardPass() = default;

	public:
		void Initialize() override;
		void Execute() override;

		void SetResourceGPUHandle(CD3DX12_GPU_DESCRIPTOR_HANDLE handle) override;

	public:
		// #TODO(Temp. Final architecture won't need this)
		void SetRenderer(Renderer* renderer);
		void SetDevice(DX12GFXDevice* device);

		void PrepareLights(const std::vector<Renderer::LightItemProtocol>& lights, const U32* lightCounts);

	private:
		void CreateRootSignature();
		void CreatePipelineState();

		void Begin(ID3D12GraphicsCommandList* commandList);
		void End(ID3D12GraphicsCommandList* commandList);

	private:
		DX12GFXDevice* mDevice;
		Renderer* mRenderer;

		D3D12_VIEWPORT* mViewport;
		D3D12_RECT mScissorRect;

		std::unique_ptr<DX12GFXDepthStencilBuffer> mDepthStencilBuffer;

		ComPtr<ID3D12RootSignature> mRootSignature;
		ComPtr<ID3D12PipelineState> mPipelineState;

		CD3DX12_GPU_DESCRIPTOR_HANDLE mInputResourceHandle;

		U32 mLightConstantBuffer;
		U32 mPerFramePixelShaderConstants; // Per frame data

		U32 mCommandList;
	};

}