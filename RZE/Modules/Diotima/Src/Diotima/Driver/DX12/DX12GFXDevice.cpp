#include <Diotima/Driver/DX12/DX12GFXDevice.h>


#include <D3DCompiler.h>

#include <Utils/Conversions.h>
#include <Utils/DebugUtils/Debug.h>
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

		mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCommandAllocator));

		InitializeAssets();
	}

	void DX12GFXDevice::SetWindow(void* windowHandle)
	{
		mWindowHandle = windowHandle;
	}

	void DX12GFXDevice::Present()
	{
		PopulateCommandList();

		ID3D12CommandList* ppCommandLists[] = { mCommandList.Get() };
		mCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		mSwapChain->Present(1, 0);

		WaitForPreviousFrame();
	}

	void DX12GFXDevice::InitializeAssets()
	{
		// ROOT SIGNATURE
		{
			CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
			rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

			ComPtr<ID3DBlob> signature;
			ComPtr<ID3DBlob> error;

			D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
			mDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&mRootSignature));
		}

		// PIPELINE STATE
		{
			ComPtr<ID3DBlob> vertexShader;
			ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
			U32 compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
			U32 compileFlags = 0;
#endif

			FilePath vertexShaderFilePath("Assets/Shaders/D3DTest.VS");
			FilePath pixelShaderFilePath("Assets/Shaders/D3DTest.PS");

			HRESULT result = D3DCompileFromFile(Conversions::StringToWString(vertexShaderFilePath.GetAbsolutePath()).c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr);
			D3DCompileFromFile(Conversions::StringToWString(pixelShaderFilePath.GetAbsolutePath()).c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr);

			D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
			{
				{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
			};

			D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
			psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
			psoDesc.pRootSignature = mRootSignature.Get();
			psoDesc.VS = { reinterpret_cast<U8*>(vertexShader->GetBufferPointer()), vertexShader->GetBufferSize() };
			psoDesc.PS = { reinterpret_cast<U8*>(pixelShader->GetBufferPointer()), pixelShader->GetBufferSize() };
			psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
			psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			psoDesc.DepthStencilState.DepthEnable = FALSE;
			psoDesc.DepthStencilState.StencilEnable = FALSE;
			psoDesc.SampleMask = UINT_MAX;
			psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
			psoDesc.SampleDesc.Count = 1;
			mDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPipelineState));
		}

		mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocator.Get(), mPipelineState.Get(), IID_PPV_ARGS(&mCommandList));
		mCommandList->Close();

		// VERTEX BUFFER
		ComPtr<ID3D12Resource> uploadBuf;
		{
			struct Vertex
			{
				Vector3D position;
				Vector4D color;
			};

			const float aspectRatio = static_cast<float>(kBufferWidth) / static_cast<float>(kBufferHeight);
			Vertex triangleVertices[] =
			{
				{ { 0.0f, 0.25f * aspectRatio, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
				{ { 0.25f, -0.25f * aspectRatio, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
				{ { -0.25f, -0.25f * aspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
			};

			const U32 vertexBufferSize = sizeof(triangleVertices);

			mCommandList->Reset(mCommandAllocator.Get(), mPipelineState.Get());

			HRESULT res = mDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuf));
			res = mDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize), D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&mVertexBuffer));

			mCommandList->ResourceBarrier(1,
				&CD3DX12_RESOURCE_BARRIER::Transition(mVertexBuffer.Get(),
					D3D12_RESOURCE_STATE_COMMON,
					D3D12_RESOURCE_STATE_COPY_DEST));

			// Copy to buffer
			U8* pVertexDataBegin;
			CD3DX12_RANGE readRange(0, 0);
			uploadBuf->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
			memcpy(pVertexDataBegin, triangleVertices, vertexBufferSize);
			uploadBuf->Unmap(0, nullptr);

			D3D12_SUBRESOURCE_DATA subResourceData = {};
			subResourceData.pData = pVertexDataBegin;
			subResourceData.RowPitch = vertexBufferSize;
			subResourceData.SlicePitch = subResourceData.RowPitch;

			UpdateSubresources<1>(mCommandList.Get(), mVertexBuffer.Get(), uploadBuf.Get(), 0, 0, 1, &subResourceData);
			//mCommandList->CopyResource(mVertexBuffer.Get(), uploadBuf.Get());

			mCommandList->ResourceBarrier(1,
				&CD3DX12_RESOURCE_BARRIER::Transition(mVertexBuffer.Get(),
					D3D12_RESOURCE_STATE_COPY_DEST,
					D3D12_RESOURCE_STATE_GENERIC_READ));

			mCommandList->Close();

			ID3D12CommandList* ppCommandLists[] = { mCommandList.Get() };
			mCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

			// Initialize vertex buffer view
			mVertexBufferView = new D3D12_VERTEX_BUFFER_VIEW();
			mVertexBufferView->BufferLocation = mVertexBuffer->GetGPUVirtualAddress();
			mVertexBufferView->StrideInBytes = sizeof(Vertex);
			mVertexBufferView->SizeInBytes = vertexBufferSize;
		}

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

		// CREATE SYNC OBJECTS TO WAIT FOR DATA TO UPLOAD TO GPU
		{
			mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence));
			mFenceValue = 1;

			mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		}

		WaitForPreviousFrame();
	}

	void DX12GFXDevice::PopulateCommandList()
	{
		mCommandAllocator->Reset();
		mCommandList->Reset(mCommandAllocator.Get(), mPipelineState.Get());

		mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
		mCommandList->RSSetViewports(1, mViewport);
		mCommandList->RSSetScissorRects(1, &mScissorRect);

		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRenderTargets[mCurrentFrame].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(mRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), mCurrentFrame, mRTVDescriptorSize);
		mCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

		const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
		mCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
		mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		mCommandList->IASetVertexBuffers(0, 1, mVertexBufferView);
		mCommandList->DrawInstanced(3, 1, 0, 0);

		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRenderTargets[mCurrentFrame].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
		mCommandList->Close();
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
}