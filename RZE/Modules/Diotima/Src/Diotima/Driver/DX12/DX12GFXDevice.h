#pragma once

#include <memory>

#include <Diotima/Driver/DX12/DX12.h>

#include <Diotima/Driver/GFXDevice.h> 

#include <Utils/PrimitiveDefs.h>

namespace Diotima
{
	class DX12GFXDepthStencilBuffer;
	class DX12GFXConstantBuffer;
	class DX12GFXIndexBuffer;
	class DX12GFXVertexBuffer;
	class DX12GFXTextureBuffer2D;

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

		virtual U32 CreateVertexBuffer(void* data, U32 numElements) override;
		virtual U32 CreateIndexBuffer(void* data, U32 numElements) override;
		virtual U32 CreateTextureBuffer2D(void* data, U32 width, U32 height) override;
		virtual U32 CreateConstantBuffer(size_t memberSize, U32 maxMembers) override;

	public:
		void BeginFrame();
		void EndFrame();
		void Present();

		ID3D12Device* GetDevice();
		ID3D12GraphicsCommandList* GetCommandList();
		ID3D12CommandQueue* GetCommandQueue();

		DX12GFXVertexBuffer* GetVertexBuffer(U32 index);
		DX12GFXIndexBuffer* GetIndexBuffer(U32 index);
		DX12GFXTextureBuffer2D* GetTextureBuffer2D(U32 index);
		DX12GFXConstantBuffer* GetConstantBuffer(U32 index);

		ID3D12DescriptorHeap* GetTextureHeap();

		U32 GetCBVSRVUAVDescriptorSize();

		void ResetCommandList();
		void ResetCommandAllocator();

		void ExecuteCommandList(ID3D12GraphicsCommandList* commandList);

		void WaitForPreviousFrame();

		void SetMSAASampleCount(U32 sampleCount);
		U32 GetMSAASampleCount();

		// #TODO(Josh::Tease this out later)
		void GenerateMipsForTexture(DX12GFXTextureBuffer2D* texture);

	private:
		void InitializeAssets();
		void CreateRootSignature();

		void InitializeMSAA();
		void CreateTextureHeap();

		void InitializeMipGeneration();
		void CreateMipUAVHeap();
		void CreateMipGenRootSignature();
		void CreateMipGenPSO();

	private:
#if defined(_DEBUG)
		ComPtr<ID3D12Debug> mDebugController;
#endif

		ComPtr<ID3D12Device> mDevice;

		ComPtr<IDXGIFactory4> mFactory;
		ComPtr<IDXGISwapChain3> mSwapChain;

		ComPtr<ID3D12CommandQueue> mCommandQueue;
		ComPtr<ID3D12CommandAllocator> mCommandAllocator;
		ComPtr<ID3D12GraphicsCommandList> mCommandList;
		
		ComPtr<ID3D12Resource> mRenderTargets[kBufferCount];
		ComPtr<ID3D12Resource> mMSAARenderTarget;

		ComPtr<ID3D12RootSignature> mRootSignature;
		ComPtr<ID3D12PipelineState> mPipelineState;

		ComPtr<ID3D12RootSignature> mMipGenRootSig;
		ComPtr<ID3D12PipelineState> mMipGenPSO;

		D3D12_VIEWPORT* mViewport;
		D3D12_RECT mScissorRect;

		// #TODO(Josh::I really don't feel like this is the intended best usage of this pattern.
		//             Definitely a major part of the cleanup pass here.)
		ComPtr<ID3D12DescriptorHeap> mRTVDescriptorHeap;
		ComPtr<ID3D12DescriptorHeap> mMSAARTVDescriptorHeap;
		ComPtr<ID3D12DescriptorHeap> mTextureHeap;
		ComPtr<ID3D12DescriptorHeap> mMipUAVHeap;

		int mCurrentFrame;
		void* mWindowHandle;

		U32 mRTVDescriptorSize;
		U32 mCBVSRVUAVDescriptorSize;

		// Fence stuff
		HANDLE mFenceEvent;
		ComPtr<ID3D12Fence> mFence;
		U64 mFenceValue;

		// NEW
	private:
		std::unique_ptr<DX12GFXDepthStencilBuffer> mDepthStencilBuffer;

		std::vector<std::unique_ptr<DX12GFXVertexBuffer>> mVertexBuffers;
		std::vector<std::unique_ptr<DX12GFXIndexBuffer>> mIndexBuffers;
		std::vector<std::unique_ptr<DX12GFXConstantBuffer>> mConstantBuffers;
		std::vector <std::unique_ptr<DX12GFXTextureBuffer2D>> m2DTextureBuffers;

		U32 mSampleCount;
	};
}