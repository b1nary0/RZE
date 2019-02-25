#pragma once

#include <memory>

#include <Diotima/Driver/DX12/DX12.h>

#include <Diotima/Driver/GFXDevice.h> 

#include <Utils/PrimitiveDefs.h>

namespace Diotima
{
	class DX12GFXDepthStencilBuffer;
	class DX12GFXIndexBuffer;
	class DX12GFXVertexBuffer;

	// #TODO(Josh::Eventually move these into a more configurable/stateful place)
	constexpr int kBufferCount = 2;
	constexpr int kBufferWidth = 1920;
	constexpr int kBufferHeight = 1080;

	class DX12GFXDevice final : public IGFXDevice
	{
		friend class DX12GFXDriverInterface;

	public:
		DX12GFXDevice();
		virtual ~DX12GFXDevice();

	public:
		virtual void Initialize() override;
		// #TODO(Josh::Really really don't like this, fix later)
		virtual void SetWindow(void* windowHandle) override;

		virtual void Shutdown() override;

		virtual U32 CreateVertexBuffer(void* data, U32 numElements);
		virtual U32 CreateIndexBuffer(void* data, U32 numElements);

	public:
		void BeginFrame();
		void EndFrame();
		void Present();

		ID3D12Device* GetDevice();
		ID3D12GraphicsCommandList* GetCommandList();
		ID3D12CommandQueue* GetCommandQueue();

		DX12GFXVertexBuffer* GetVertexBuffer(U32 index);
		DX12GFXIndexBuffer* GetIndexBuffer(U32 index);

		void ResetCommandList();
		void ResetCommandAllocator();

		void WaitForPreviousFrame();

	private:
		void InitializeAssets();

	private:
		ComPtr<ID3D12Device> mDevice;
		ComPtr<ID3D12CommandQueue> mCommandQueue;
		ComPtr<ID3D12CommandAllocator> mCommandAllocator;
		ComPtr<ID3D12DescriptorHeap> mRTVDescriptorHeap;
		ComPtr<ID3D12Resource> mRenderTargets[kBufferCount];
		ComPtr<ID3D12RootSignature> mRootSignature;
		ComPtr<ID3D12PipelineState> mPipelineState;
		ComPtr<ID3D12GraphicsCommandList> mCommandList;

		ComPtr<IDXGIFactory4> mFactory;
		ComPtr<IDXGISwapChain3> mSwapChain;

		ComPtr<ID3D12Resource> mVertexBuffer;

		D3D12_VIEWPORT* mViewport;
		D3D12_RECT mScissorRect;
		D3D12_VERTEX_BUFFER_VIEW* mVertexBufferView;

		int mCurrentFrame;
		void* mWindowHandle;

		U32 mRTVDescriptorSize;

		// Fence stuff
		HANDLE mFenceEvent;
		ComPtr<ID3D12Fence> mFence;
		U64 mFenceValue;

		// NEW
	private:
		std::unique_ptr<DX12GFXDepthStencilBuffer> mDepthStencilBuffer;
		std::vector<std::unique_ptr<DX12GFXVertexBuffer>> mVertexBuffers;
		std::vector<std::unique_ptr<DX12GFXIndexBuffer>> mIndexBuffers;

	};
}