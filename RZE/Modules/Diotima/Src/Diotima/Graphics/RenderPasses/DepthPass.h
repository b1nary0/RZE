#pragma once

#include <Diotima/Graphics/RenderPass.h>

#include <Diotima/Renderer.h>

#include <Diotima/Driver/DX12/DX12.h>

namespace Diotima
{
	class DX12GFXDevice;
	class DX12GFXDepthStencilBuffer;

	class DepthPass final : public GFXRenderPass
	{
	public:
		// #TODO(device parameter should eventually be a context (cmd list of its own)
		DepthPass();
		virtual ~DepthPass();

	public:
		void Initialize(int width, int height) override;
		void Execute() override;

		void OnWindowResize(int newWidth, int newHeight) override {}

		// #TODO(Temp. Final architecture won't need this)
		void SetRenderer(Renderer* renderer);
		void SetDevice(DX12GFXDevice* device);

	private:
		void Begin();
		void End();

		void PrepareLights(const std::vector<Renderer::LightItemProtocol>& lights);

	private:
		DX12GFXDevice* mDevice;
		Renderer* mRenderer;

		D3D12_VIEWPORT* mViewport;
		D3D12_RECT mScissorRect;

		std::unique_ptr<DX12GFXDepthStencilBuffer> mDepthStencilBuffer;

		ComPtr<ID3D12Resource> mDepthTexture;
		CD3DX12_CPU_DESCRIPTOR_HANDLE mDepthTexCPUHandle;
		CD3DX12_GPU_DESCRIPTOR_HANDLE mDepthTexGPUHandle;

		ComPtr<ID3D12RootSignature> mRootSignature;
		ComPtr<ID3D12PipelineState> mPipelineState;

		U32 mLightConstantBuffer;
		U32 mWorldMatrixBuffer;

		U32 mCommandList;
	};

}