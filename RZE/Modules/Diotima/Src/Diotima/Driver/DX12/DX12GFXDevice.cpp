#include <Diotima/Driver/DX12/DX12GFXDevice.h>

#include <Diotima/Driver/DX12/DX12GFXDepthStencilBuffer.h>
#include <Diotima/Driver/DX12/DX12GFXConstantBuffer.h>
#include <Diotima/Driver/DX12/DX12GFXIndexBuffer.h>
#include <Diotima/Driver/DX12/DX12GFXVertexBuffer.h>
#include <Diotima/Driver/DX12/DX12GFXTextureBuffer2D.h>

#include <Utils/Conversions.h>
#include <Utils/DebugUtils/Debug.h>
#include <Utils/Math/Vector2D.h>
#include <Utils/Math/Vector3D.h>
#include <Utils/Math/Vector4D.h>
#include <Utils/Platform/FilePath.h>

#include <algorithm>

namespace
{
	void GetHardwareAdapter(IDXGIFactory4* pFactory, IDXGIAdapter1** ppAdapter)
	{
		*ppAdapter = nullptr;
		for (UINT adapterIndex = 0; ; ++adapterIndex)
		{
			IDXGIAdapter1* pAdapter = nullptr;
			if (DXGI_ERROR_NOT_FOUND == pFactory->EnumAdapters1(adapterIndex, &pAdapter))
			{
				// No more adapters to enumerate.
				break;
			}

			// Check to see if the adapter supports Direct3D 12, but don't create the
			// actual device yet.
			if (SUCCEEDED(D3D12CreateDevice(pAdapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
			{
				DXGI_ADAPTER_DESC adapterDesc;
				pAdapter->GetDesc(&adapterDesc);

				LOG_CONSOLE_ARGS("GPU: %ls", adapterDesc.Description);

				*ppAdapter = pAdapter;
				return;
			}
			pAdapter->Release();
		}
	}
}

namespace Diotima
{

	DX12GFXDevice::DX12GFXDevice()
		: mSampleCount(1)
	{
	}

	DX12GFXDevice::~DX12GFXDevice()
	{
	}

	void DX12GFXDevice::Initialize()
	{
#if defined(_DEBUG)
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&mDebugController))))
		{
			mDebugController->EnableDebugLayer();
		}
#endif

		// #TODO(Josh::Check for errors)
		CreateDXGIFactory1(IID_PPV_ARGS(&mFactory));

		ComPtr<IDXGIAdapter1> hardwareAdapter;
		GetHardwareAdapter(mFactory.Get(), &hardwareAdapter);

		// #TODO(Josh::Check for errors)
		D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&mDevice));

		// CREATE SYNC OBJECTS TO WAIT FOR DATA TO UPLOAD TO GPU
		{
			HRESULT result = mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence));
			mFenceValue = 1;

			mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		}

		// COMMAND QUEUE
		{
			D3D12_COMMAND_QUEUE_DESC queueDesc = {};
			queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

			// #TODO(Josh::Check for errors)
			mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue));
		}

		// SWAP CHAIN
		{
			DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
			swapChainDesc.BufferCount = kBufferCount;
			swapChainDesc.BufferDesc.Width = kBufferWidth;
			swapChainDesc.BufferDesc.Height = kBufferHeight;
			swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			swapChainDesc.OutputWindow = reinterpret_cast<HWND>(mWindowHandle);
			swapChainDesc.SampleDesc.Count = 1;
			swapChainDesc.Windowed = TRUE;

			ComPtr<IDXGISwapChain> swapChain;
			// #TODO(Josh::Check for errors)
			mFactory->CreateSwapChain(mCommandQueue.Get(), &swapChainDesc, &swapChain);
			// #TODO(Josh::Check for errors)
			swapChain.As(&mSwapChain);
		}

		mFactory->MakeWindowAssociation(reinterpret_cast<HWND>(mWindowHandle), DXGI_MWA_NO_ALT_ENTER);

		mCurrentFrame = mSwapChain->GetCurrentBackBufferIndex();

		// DESCRIPTOR HEAP
		{
			D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
			rtvHeapDesc.NumDescriptors = kBufferCount;
			rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

			mDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&mRTVDescriptorHeap));
			mRTVDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		}

		// TEXTURE HEAP
		CreateTextureHeap();

		// RENDER TARGETS (FRAMES)
		{
			CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(mRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

			for (U32 frameIndex = 0; frameIndex < kBufferCount; ++frameIndex)
			{
				mSwapChain->GetBuffer(frameIndex, IID_PPV_ARGS(&mRenderTargets[frameIndex]));
				mDevice->CreateRenderTargetView(mRenderTargets[frameIndex].Get(), nullptr, rtvHandle);
				rtvHandle.Offset(1, mRTVDescriptorSize);
			}
		}

		{
			// CREATE COMMAND ALLOCATORS
			mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCommandAllocator));
			mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mResourceCommandAllocator));
		}

		InitializeMSAA(kBufferWidth, kBufferHeight);

		InitializeMipGeneration();
	}

	void DX12GFXDevice::SetWindow(void* windowHandle)
	{
		mWindowHandle = windowHandle;
	}

	void DX12GFXDevice::Present()
	{
		mSwapChain->Present(mSyncInterval, 0);

		WaitForPreviousFrame();
	}

	void DX12GFXDevice::WaitForPreviousFrame()
	{
		const U64 fence = mFenceValue;
		HRESULT res = mCommandQueue->Signal(mFence.Get(), fence);
		++mFenceValue;

		if (mFence->GetCompletedValue() < fence)
		{
			mFence->SetEventOnCompletion(fence, mFenceEvent);
			WaitForSingleObject(mFenceEvent, INFINITE);
		}

		mCurrentFrame = mSwapChain->GetCurrentBackBufferIndex();
	}

	void DX12GFXDevice::Shutdown()
	{
		WaitForPreviousFrame();
		CloseHandle(mFenceEvent);
	}

	U32 DX12GFXDevice::CreateVertexBuffer(void* data, U32 numElements)
	{
		mVertexBuffers.push_back(std::make_unique<DX12GFXVertexBuffer>());
		mVertexBuffers.back()->SetDevice(this);
		mVertexBuffers.back()->Allocate(data, numElements);

		return static_cast<U32>(mVertexBuffers.size() - 1);
	}

	U32 DX12GFXDevice::CreateIndexBuffer(void* data, U32 numElements)
	{
		mIndexBuffers.push_back(std::make_unique<DX12GFXIndexBuffer>());
		mIndexBuffers.back()->SetDevice(this);
		mIndexBuffers.back()->Allocate(data, numElements);

		return static_cast<U32>(mIndexBuffers.size() - 1);
	}

	U32 DX12GFXDevice::CreateTextureBuffer2D(void* data, U32 width, U32 height)
	{
		m2DTextureBuffers.push_back(std::make_unique<DX12GFXTextureBuffer2D>());
		m2DTextureBuffers.back()->SetDevice(this);
		m2DTextureBuffers.back()->Allocate(data, width, height);

		return static_cast<U32>(m2DTextureBuffers.size() - 1);
	}

	U32 DX12GFXDevice::CreateConstantBuffer(size_t memberSize, U32 maxMembers)
	{
		mConstantBuffers.push_back(std::make_unique<DX12GFXConstantBuffer>());
		mConstantBuffers.back()->SetDevice(this);
		mConstantBuffers.back()->Allocate(memberSize, maxMembers);

		return static_cast<U32>(mConstantBuffers.size() - 1);
	}

	ID3D12Device* DX12GFXDevice::GetDevice()
	{
		return mDevice.Get();
	}

	ID3D12CommandQueue* DX12GFXDevice::GetCommandQueue()
	{
		return mCommandQueue.Get();
	}

	Diotima::DX12GFXVertexBuffer* DX12GFXDevice::GetVertexBuffer(U32 index)
	{
		AssertExpr(index < mVertexBuffers.size());
		return mVertexBuffers[index].get();
	}

	Diotima::DX12GFXIndexBuffer* DX12GFXDevice::GetIndexBuffer(U32 index)
	{
		AssertExpr(index < mIndexBuffers.size());
		return mIndexBuffers[index].get();
	}

	void DX12GFXDevice::ResetCommandAllocator()
	{
		mCommandAllocator->Reset();
	}

	Diotima::DX12GFXTextureBuffer2D* DX12GFXDevice::GetTextureBuffer2D(U32 index)
	{
		return m2DTextureBuffers[index].get();
	}

	Diotima::DX12GFXConstantBuffer* DX12GFXDevice::GetConstantBuffer(U32 index)
	{
		return mConstantBuffers[index].get();
	}

	void DX12GFXDevice::CreateTextureHeap()
	{
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.NumDescriptors = 1024;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		mDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&mTextureHeap));

		mCBVSRVUAVDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	ID3D12DescriptorHeap* DX12GFXDevice::GetTextureHeap()
	{
		return mTextureHeap.Get();
	}

	U32 DX12GFXDevice::GetCBVSRVUAVDescriptorSize()
	{
		return mCBVSRVUAVDescriptorSize;
	}

	void DX12GFXDevice::InitializeMSAA(int width, int height)
	{
		// Create descriptor heaps for MSAA render target views and depth stencil views.
		D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc = {};
		rtvDescriptorHeapDesc.NumDescriptors = 1;
		rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

		mDevice->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(&mMSAARTVDescriptorHeap));

		D3D12_RESOURCE_DESC msaaRTDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, width, height, 1, 1, mSampleCount);
		msaaRTDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		D3D12_CLEAR_VALUE  msaaOptimizedClearValue = {};
		msaaOptimizedClearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

		mDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), 
			D3D12_HEAP_FLAG_NONE, 
			&msaaRTDesc, 
			D3D12_RESOURCE_STATE_RESOLVE_SOURCE, 
			&msaaOptimizedClearValue,
			IID_PPV_ARGS(&mMSAARenderTarget));

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = DXGI_FORMAT_UNKNOWN;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;

		mDevice->CreateRenderTargetView(mMSAARenderTarget.Get(), &rtvDesc, mMSAARTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	}

	void DX12GFXDevice::SetMSAASampleCount(U32 sampleCount)
	{
		AssertExpr(sampleCount > 1);
		mSampleCount = sampleCount;
	}

	U32 DX12GFXDevice::GetMSAASampleCount()
	{
		return mSampleCount;
	}

	void DX12GFXDevice::GenerateMipsForTexture(DX12GFXTextureBuffer2D* texture)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srcSRVDesc = {};
		srcSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srcSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;

		D3D12_UNORDERED_ACCESS_VIEW_DESC dstUAVDesc = {};
		dstUAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

		ID3D12GraphicsCommandList* commandList = GetGraphicsCommandList(CreateGraphicsCommandList(GetResourceCommandAllocator(), nullptr));

		commandList->SetComputeRootSignature(mMipGenRootSig.Get());
		commandList->SetPipelineState(mMipGenPSO.Get());

		ID3D12DescriptorHeap* descriptorHeaps[] = { mMipUAVHeap.Get() };
		commandList->SetDescriptorHeaps(1, descriptorHeaps);

		CD3DX12_CPU_DESCRIPTOR_HANDLE currCPUHandle(mMipUAVHeap->GetCPUDescriptorHandleForHeapStart(), 0, mCBVSRVUAVDescriptorSize);
		CD3DX12_GPU_DESCRIPTOR_HANDLE currGPUHandle(mMipUAVHeap->GetGPUDescriptorHandleForHeapStart(), 0, mCBVSRVUAVDescriptorSize);

		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(texture->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));

		const D3D12_RESOURCE_DESC& srcTextureDesc = texture->GetResourceDesc();
		for (U32 srcMipLevel = 0; srcMipLevel < 3; ++srcMipLevel)
		{
			U32 dstWidth = std::max<U32>(static_cast<U32>(srcTextureDesc.Width >> (srcMipLevel + 1)), 1);
			U32 dstHeight = std::max<U32>(static_cast<U32>(srcTextureDesc.Height >> (srcMipLevel + 1)), 1);

			srcSRVDesc.Format = srcTextureDesc.Format;
			srcSRVDesc.Texture2D.MipLevels = 1;
			srcSRVDesc.Texture2D.MostDetailedMip = srcMipLevel;
			mDevice->CreateShaderResourceView(texture->GetResource(), &srcSRVDesc, currCPUHandle);
			currCPUHandle.Offset(1, mCBVSRVUAVDescriptorSize);

			dstUAVDesc.Format = srcTextureDesc.Format;
			dstUAVDesc.Texture2D.MipSlice = srcMipLevel + 1;
			mDevice->CreateUnorderedAccessView(texture->GetResource(), nullptr, &dstUAVDesc, currCPUHandle);
			currCPUHandle.Offset(1, mCBVSRVUAVDescriptorSize);

			struct MipCBData
			{
				Vector2D TexelSize;
			} mipCBData;

			mipCBData.TexelSize.SetXY(1.0f / dstWidth, 1.0f / dstHeight);

			commandList->SetComputeRoot32BitConstants(0, 2, &mipCBData, 0);

			commandList->SetComputeRootDescriptorTable(1, currGPUHandle);
			currGPUHandle.Offset(1, mCBVSRVUAVDescriptorSize);
			commandList->SetComputeRootDescriptorTable(2, currGPUHandle);
			currGPUHandle.Offset(1, mCBVSRVUAVDescriptorSize);

			commandList->Dispatch(std::max<U32>(dstWidth / 8, 1u), std::max<U32>(dstHeight / 8, 1u), 1);

			commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(texture->GetResource()));
		}

		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(texture->GetResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

		ExecuteCommandList(commandList);
	}

	void DX12GFXDevice::InitializeMipGeneration()
	{
		// Need a descriptor heap
		CreateMipUAVHeap();
		// Need a root signature
		CreateMipGenRootSignature();
		// Need a PSO
		CreateMipGenPSO();

		// Need n UAVs for n mip levels
	}

	void DX12GFXDevice::CreateMipGenRootSignature()
	{
		CD3DX12_DESCRIPTOR_RANGE1 srvRanges[2];
		srvRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);
		srvRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);

		CD3DX12_ROOT_PARAMETER1 rootParams[3];
		rootParams[0].InitAsConstants(2, 0);
		rootParams[1].InitAsDescriptorTable(1, &srvRanges[0]);
		rootParams[2].InitAsDescriptorTable(1, &srvRanges[1]);

		CD3DX12_STATIC_SAMPLER_DESC linearClampSampler(
			0,
			D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT,
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP
		);

		linearClampSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSigDesc(_countof(rootParams), rootParams, 1, &linearClampSampler);
		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;

		D3DX12SerializeVersionedRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1_1, &signature, &error);
		if (error != nullptr)
		{
			OutputDebugStringA((char*)error->GetBufferPointer());
		}

		HRESULT res = mDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&mMipGenRootSig));
	}

	void DX12GFXDevice::CreateMipGenPSO()
	{
		ComPtr<ID3DBlob> computeShader;

#if defined(_DEBUG)
		U32 compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		U32 compileFlags = 0;
#endif

		FilePath computeShaderPath("Assets/Shaders/GenerateMips.hlsl");

		ComPtr<ID3DBlob> error;

		HRESULT result = D3DCompileFromFile(Conversions::StringToWString(computeShaderPath.GetAbsolutePath()).c_str(), nullptr, nullptr, "CSMain", "cs_5_1", compileFlags, 0, &computeShader, &error);
		if (error)
		{
			OutputDebugStringA((char*)error->GetBufferPointer());
		}

		D3D12_COMPUTE_PIPELINE_STATE_DESC computePSDesc = {};
		computePSDesc.CS = { reinterpret_cast<U8*>(computeShader->GetBufferPointer()), computeShader->GetBufferSize() };
		computePSDesc.pRootSignature = mMipGenRootSig.Get();

		mDevice->CreateComputePipelineState(&computePSDesc, IID_PPV_ARGS(&mMipGenPSO));
	}

	void DX12GFXDevice::CreateMipUAVHeap()
	{
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.NumDescriptors = 8;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		mDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&mMipUAVHeap));

		// #TODO(Josh::Move this out of here and CreateTextureHeap() into some init function)
		mCBVSRVUAVDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	void DX12GFXDevice::ExecuteCommandList(ID3D12GraphicsCommandList* commandList)
	{
		commandList->Close();

		ID3D12CommandList* ppCommandLists[] = { commandList };
		mCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		WaitForPreviousFrame();
	}

	ID3D12CommandAllocator* DX12GFXDevice::GetCommandAllocator()
	{
		return mCommandAllocator.Get();
	}

	U32 DX12GFXDevice::CreateGraphicsCommandList(ID3D12CommandAllocator* allocator, ID3D12PipelineState* pipelineState)
	{
		ComPtr<ID3D12GraphicsCommandList> commandList;
		HRESULT result = mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator, pipelineState, IID_PPV_ARGS(&commandList));

		U32 retID = mGraphicsCommandLists.size();
		mGraphicsCommandLists.push_back(commandList);

		return retID;
	}

	ID3D12GraphicsCommandList* DX12GFXDevice::GetGraphicsCommandList(U32 index)
	{
		AssertExpr(index < mGraphicsCommandLists.size());
		return mGraphicsCommandLists[index].Get();
	}

	ID3D12CommandAllocator* DX12GFXDevice::GetResourceCommandAllocator()
	{
		return mResourceCommandAllocator.Get();
	}

	void DX12GFXDevice::ResetResourceCommandAllocator()
	{
		mResourceCommandAllocator->Reset();
	}

	ID3D12DescriptorHeap* DX12GFXDevice::GetRenderTargetHeap()
	{
		return mRTVDescriptorHeap.Get();
	}

	U32 DX12GFXDevice::GetRTVDescriptorSize()
	{
		return mRTVDescriptorSize;
	}

	int DX12GFXDevice::GetCurrentFrame()
	{
		return mCurrentFrame;
	}

	ID3D12Resource* DX12GFXDevice::GetCurrentFrameRenderTarget()
	{
		return mRenderTargets[mCurrentFrame].Get();
	}

	ID3D12Resource* DX12GFXDevice::GetMSAARenderTarget()
	{
		return mMSAARenderTarget.Get();
	}

	ID3D12DescriptorHeap* DX12GFXDevice::GetMSAARTVHeap()
	{
		return mMSAARTVDescriptorHeap.Get();
	}

	void DX12GFXDevice::HandleWindowResize(int newWidth, int newHeight)
	{
		for (int bufferIndex = 0; bufferIndex < kBufferCount; ++bufferIndex)
		{
			mRenderTargets[bufferIndex].Reset();
		}

		DXGI_SWAP_CHAIN_DESC swapChainDesc;
		mSwapChain->GetDesc(&swapChainDesc);

		mSwapChain->ResizeBuffers(kBufferCount, newWidth, newHeight, swapChainDesc.BufferDesc.Format, swapChainDesc.Flags);

		// RENDER TARGETS (FRAMES)
		{
			CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(mRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

			for (U32 frameIndex = 0; frameIndex < kBufferCount; ++frameIndex)
			{
				mSwapChain->GetBuffer(frameIndex, IID_PPV_ARGS(&mRenderTargets[frameIndex]));
				mDevice->CreateRenderTargetView(mRenderTargets[frameIndex].Get(), nullptr, rtvHandle);
				rtvHandle.Offset(1, mRTVDescriptorSize);
			}
		}

		mMSAARenderTarget.Reset();
		mMSAARTVDescriptorHeap.Reset();
		InitializeMSAA(newWidth, newHeight);
	}

	void DX12GFXDevice::SetSyncInterval(U32 syncInterval)
	{
		mSyncInterval = syncInterval;
	}

	U32 DX12GFXDevice::GetVertexBufferCount() const
	{
		return mVertexBuffers.size();
	}

	U32 DX12GFXDevice::GetIndexBufferCount() const
	{
		return mIndexBuffers.size();
	}

	U32 DX12GFXDevice::GetTextureBufferCount() const
	{
		return m2DTextureBuffers.size();
	}

}