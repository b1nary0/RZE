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
	constexpr int kBufferWidth = 1584;
	constexpr int kBufferHeight = 861;

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

		U32 CreateVertexBuffer(void* data, U32 numElements) override;
		U32 CreateIndexBuffer(void* data, U32 numElements) override;
		U32 CreateTextureBuffer2D(void* data, U32 width, U32 height) override;
		U32 CreateConstantBuffer(size_t memberSize, U32 maxMembers) override;

		U32 CreateGraphicsCommandList(ID3D12CommandAllocator* allocator, ID3D12PipelineState* pipelineState);

		U32 GetVertexBufferCount() const;
		U32 GetIndexBufferCount() const;
		U32 GetTextureBufferCount() const;

	public:
		void Present();

		ID3D12Device* GetDevice();
		ID3D12CommandQueue* GetCommandQueue();
		ID3D12CommandAllocator* GetCommandAllocator();
		ID3D12CommandAllocator* GetResourceCommandAllocator();

		DX12GFXVertexBuffer* GetVertexBuffer(U32 index);
		DX12GFXIndexBuffer* GetIndexBuffer(U32 index);
		DX12GFXTextureBuffer2D* GetTextureBuffer2D(U32 index);
		DX12GFXConstantBuffer* GetConstantBuffer(U32 index);

		ID3D12GraphicsCommandList* GetGraphicsCommandList(U32 index);

		ID3D12DescriptorHeap* GetTextureHeap();
		ID3D12DescriptorHeap* GetRenderTargetHeap();
		ID3D12DescriptorHeap* GetMSAARTVHeap();

		U32 GetRTVDescriptorSize();
		U32 GetCBVSRVUAVDescriptorSize();

		int GetCurrentFrame();
		ID3D12Resource* GetCurrentFrameRenderTarget();
		ID3D12Resource* GetMSAARenderTarget();

		void ResetCommandAllocator();
		void ResetResourceCommandAllocator();

		void ExecuteCommandList(ID3D12GraphicsCommandList* commandList);

		void WaitForPreviousFrame();

		void SetSyncInterval(U32 syncInterval);

		void SetMSAASampleCount(U32 sampleCount);
		U32 GetMSAASampleCount();

		// #TODO(Josh::Tease this out later)
		void GenerateMipsForTexture(DX12GFXTextureBuffer2D* texture);

		void HandleWindowResize(int newWidth, int newHeight);

	private:
		void InitializeMSAA(int width, int height);
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
		ComPtr <ID3D12CommandAllocator> mResourceCommandAllocator;
		
		ComPtr<ID3D12Resource> mRenderTargets[kBufferCount];
		ComPtr<ID3D12Resource> mMSAARenderTarget;

		ComPtr<ID3D12RootSignature> mMipGenRootSig;
		ComPtr<ID3D12PipelineState> mMipGenPSO;

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
		std::vector<std::unique_ptr<DX12GFXVertexBuffer>> mVertexBuffers;
		std::vector<std::unique_ptr<DX12GFXIndexBuffer>> mIndexBuffers;
		std::vector<std::unique_ptr<DX12GFXConstantBuffer>> mConstantBuffers;
		std::vector<std::unique_ptr<DX12GFXTextureBuffer2D>> m2DTextureBuffers;

		std::vector<ComPtr<ID3D12GraphicsCommandList>> mGraphicsCommandLists;

		U32 mSampleCount;
		U32 mSyncInterval;
	};
}