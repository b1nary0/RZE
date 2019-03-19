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
		// #TODO(Josh::Check for errors)
		CreateDXGIFactory1(IID_PPV_ARGS(&mFactory));

		ComPtr<IDXGIAdapter1> hardwareAdapter;
		GetHardwareAdapter(mFactory.Get(), &hardwareAdapter);

		// #TODO(Josh::Check for errors)
		D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&mDevice));

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

		InitializeMSAA();

		mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCommandAllocator));

		InitializeAssets();
	}

	void DX12GFXDevice::SetWindow(void* windowHandle)
	{
		mWindowHandle = windowHandle;
	}

	void DX12GFXDevice::Present()
	{
		ID3D12CommandList* ppCommandLists[] = { mCommandList.Get() };
		mCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		mSwapChain->Present(1, 0);

		WaitForPreviousFrame();
	}

	void DX12GFXDevice::InitializeAssets()
	{
		// ROOT SIGNATURE
		CreateRootSignature();

		// PIPELINE STATE
		{
			ComPtr<ID3DBlob> vertexShader;
			ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
			U32 compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
			U32 compileFlags = 0;
#endif

			FilePath vertexShaderFilePath("Assets/Shaders/D3DTest_VS.hlsl");
			FilePath pixelShaderFilePath("Assets/Shaders/D3DTest_PS.hlsl");

			ComPtr<ID3DBlob> error;

			HRESULT result = D3DCompileFromFile(Conversions::StringToWString(vertexShaderFilePath.GetAbsolutePath()).c_str(), nullptr, nullptr, "VSMain", "vs_5_1", compileFlags, 0, &vertexShader, &error);
			if (error)
			{
				OutputDebugStringA((char*)error->GetBufferPointer());
			}

			D3DCompileFromFile(Conversions::StringToWString(pixelShaderFilePath.GetAbsolutePath()).c_str(), nullptr, nullptr, "PSMain", "ps_5_1", compileFlags, 0, &pixelShader, &error);
			if (error)
			{
				OutputDebugStringA((char*)error->GetBufferPointer());
			}

			D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
			{
				{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{"UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
			};

			D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
			psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
			psoDesc.pRootSignature = mRootSignature.Get();
			psoDesc.VS = { reinterpret_cast<U8*>(vertexShader->GetBufferPointer()), vertexShader->GetBufferSize() };
			psoDesc.PS = { reinterpret_cast<U8*>(pixelShader->GetBufferPointer()), pixelShader->GetBufferSize() };
			psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
			psoDesc.RasterizerState.FrontCounterClockwise = TRUE;
			psoDesc.RasterizerState.MultisampleEnable = TRUE;
			psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
			psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
			psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
			psoDesc.SampleMask = UINT_MAX;
			psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
			psoDesc.SampleDesc.Count = mSampleCount;
			mDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPipelineState));
		}

		mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocator.Get(), mPipelineState.Get(), IID_PPV_ARGS(&mCommandList));
		mCommandList->Close();

		mViewport = new D3D12_VIEWPORT();
		mViewport->Height = kBufferHeight;
		mViewport->Width = kBufferWidth;
		mViewport->TopLeftX = 0;
		mViewport->TopLeftY = 0;
		mViewport->MinDepth = 0.0f;
		mViewport->MaxDepth = 1.0f;

		mScissorRect.left = 0;
		mScissorRect.top = 0;
		mScissorRect.right = kBufferWidth;
		mScissorRect.bottom = kBufferHeight;

		mDepthStencilBuffer = std::make_unique<DX12GFXDepthStencilBuffer>();
		mDepthStencilBuffer->SetDevice(this);
		mDepthStencilBuffer->Allocate();

		// CREATE SYNC OBJECTS TO WAIT FOR DATA TO UPLOAD TO GPU
		{
			mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence));
			mFenceValue = 1;

			mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		}

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

	U32 DX12GFXDevice::CreateConstantBuffer(void* data, U32 size)
	{
		mConstantBuffers.push_back(std::make_unique<DX12GFXConstantBuffer>());
		mConstantBuffers.back()->SetDevice(this);
		mConstantBuffers.back()->Allocate(data, size);

		return static_cast<U32>(mConstantBuffers.size() - 1);
	}

	ID3D12Device* DX12GFXDevice::GetDevice()
	{
		return mDevice.Get();
	}

	ID3D12GraphicsCommandList* DX12GFXDevice::GetCommandList()
	{
		return mCommandList.Get();
	}

	ID3D12CommandQueue* DX12GFXDevice::GetCommandQueue()
	{
		return mCommandQueue.Get();
	}

	void DX12GFXDevice::ResetCommandList()
	{
		mCommandList->Reset(mCommandAllocator.Get(), mPipelineState.Get());
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

	void DX12GFXDevice::BeginFrame()
	{
		mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
		mCommandList->RSSetViewports(1, mViewport);
		mCommandList->RSSetScissorRects(1, &mScissorRect);

		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mMSAARenderTarget.Get(), D3D12_RESOURCE_STATE_RESOLVE_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(mMSAARTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), 0, mRTVDescriptorSize);
		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(mDepthStencilBuffer->GetDescriptorHeap()->GetCPUDescriptorHandleForHeapStart());
		mCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
		mCommandList->ClearDepthStencilView(mDepthStencilBuffer->GetDescriptorHeap()->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
		
		const float clearColor[] = { 0.f, 0.f, 0.f, 0.0f };
		mCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	}

	void DX12GFXDevice::EndFrame()
	{
		D3D12_RESOURCE_BARRIER barriers[2] =
		{
			CD3DX12_RESOURCE_BARRIER::Transition(
				mMSAARenderTarget.Get(),
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_RESOLVE_SOURCE),
			CD3DX12_RESOURCE_BARRIER::Transition(
				mRenderTargets[mCurrentFrame].Get(),
				D3D12_RESOURCE_STATE_PRESENT,
				D3D12_RESOURCE_STATE_RESOLVE_DEST)
		};

		mCommandList->ResourceBarrier(2, barriers);

		mCommandList->ResolveSubresource(mRenderTargets[mCurrentFrame].Get(), 0, mMSAARenderTarget.Get(), 0, DXGI_FORMAT_R8G8B8A8_UNORM);

		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
			mRenderTargets[mCurrentFrame].Get(),
			D3D12_RESOURCE_STATE_RESOLVE_DEST,
			D3D12_RESOURCE_STATE_PRESENT));

		mCommandList->Close();
	}

	void DX12GFXDevice::CreateRootSignature()
	{
		D3D12_DESCRIPTOR_RANGE1 descriptorTableRanges[1];
		descriptorTableRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		descriptorTableRanges[0].NumDescriptors = 3;
		descriptorTableRanges[0].BaseShaderRegister = 0;
		descriptorTableRanges[0].RegisterSpace = 0;
		descriptorTableRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		descriptorTableRanges[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

		D3D12_ROOT_DESCRIPTOR_TABLE1 descriptorTable;
		descriptorTable.NumDescriptorRanges = _countof(descriptorTableRanges);
		descriptorTable.pDescriptorRanges = &descriptorTableRanges[0];

		D3D12_ROOT_DESCRIPTOR1 mMVPConstBuffer;
		mMVPConstBuffer.RegisterSpace = 0;
		mMVPConstBuffer.ShaderRegister = 0;
		mMVPConstBuffer.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;

		D3D12_ROOT_DESCRIPTOR1 mLightConstBuffer;
		mLightConstBuffer.RegisterSpace = 1;
		mLightConstBuffer.ShaderRegister = 0;
		mLightConstBuffer.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;

		D3D12_ROOT_DESCRIPTOR1 mPerMeshPixelShaderConstants;
		mPerMeshPixelShaderConstants.RegisterSpace = 3;
		mPerMeshPixelShaderConstants.ShaderRegister = 0;
		mPerMeshPixelShaderConstants.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;

		D3D12_ROOT_DESCRIPTOR1 mPerFramePixelShaderConstants;
		mPerFramePixelShaderConstants.RegisterSpace = 4;
		mPerFramePixelShaderConstants.ShaderRegister = 0;
		mPerFramePixelShaderConstants.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;

		CD3DX12_ROOT_PARAMETER1 rootParameters[6];
		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[0].Descriptor = mMVPConstBuffer;
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

		rootParameters[1].InitAsConstants(3, 0, 2, D3D12_SHADER_VISIBILITY_PIXEL);

		rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[2].Descriptor = mLightConstBuffer;
		rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[3].DescriptorTable = descriptorTable;
		rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		rootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[4].Descriptor = mPerMeshPixelShaderConstants;
		rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		rootParameters[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[5].Descriptor = mPerFramePixelShaderConstants;
		rootParameters[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_STATIC_SAMPLER_DESC sampler = {};
		sampler.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.MipLODBias = 0;
		sampler.MaxAnisotropy = 0;
		sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		sampler.MinLOD = 0.0f;
		sampler.MaxLOD = D3D12_FLOAT32_MAX;
		sampler.ShaderRegister = 0;
		sampler.RegisterSpace = 0;
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init_1_1(_countof(rootParameters), &rootParameters[0], 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;

		D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_1, &signature, &error);
		if (error != nullptr)
		{
			OutputDebugStringA((char*)error->GetBufferPointer());
		}

		HRESULT res = mDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&mRootSignature));
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
		heapDesc.NumDescriptors = 32;
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

	void DX12GFXDevice::InitializeMSAA()
	{
		// Create descriptor heaps for MSAA render target views and depth stencil views.
		D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc = {};
		rtvDescriptorHeapDesc.NumDescriptors = 1;
		rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

		mDevice->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(&mMSAARTVDescriptorHeap));

		D3D12_RESOURCE_DESC msaaRTDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, kBufferWidth, kBufferHeight, 1, 1, mSampleCount);
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

	}

	void DX12GFXDevice::InitializeMipGeneration()
	{
		// Need a root signature
		CreateMipGenRootSignature();
		// Need a PSO
		CreateMipGenPSO();

		// Need n UAVs for n mip levels
	}

	void DX12GFXDevice::CreateMipGenRootSignature()
	{
		CD3DX12_DESCRIPTOR_RANGE1 srcMip(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);
		CD3DX12_DESCRIPTOR_RANGE1 outMip(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 4, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);

		CD3DX12_ROOT_PARAMETER1 rootParams[3];

		// #TODO(Josh::Move this out later)
		struct alignas(16) GenerateMipsCB
		{
			uint32_t SrcMipLevel;    // Texture level of source mip
			uint32_t NumMipLevels;	 // Number of OutMips to write: [1-4]
			uint32_t SrcDimension;	 // Width and height of the source texture are even or odd.
			//uint32_t IsSRGB;		 // Must apply gamma correction to sRGB textures.
			Vector2D TexelSize;		 // 1.0 / OutMip1.Dimensions
		};

		rootParams[0].InitAsConstants(sizeof(GenerateMipsCB) / 4, 0);
		rootParams[1].InitAsDescriptorTable(1, &srcMip);
		rootParams[2].InitAsDescriptorTable(1, &outMip);

		CD3DX12_STATIC_SAMPLER_DESC linearClampSampler(
			0,
			D3D12_FILTER_MIN_MAG_MIP_POINT,
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP
		);

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSigDesc(_countof(rootParams), rootParams, 1, &linearClampSampler);

	}

	void DX12GFXDevice::CreateMipGenPSO()
	{

	}

}