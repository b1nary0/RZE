#include <Diotima/Graphics/RenderPasses/DepthPass.h>

#include <Diotima/Driver/DX12/DX12GFXDevice.h>
#include <Diotima/Driver/DX12/DX12GFXConstantBuffer.h>
#include <Diotima/Driver/DX12/DX12GFXDepthStencilBuffer.h>
#include <Diotima/Driver/DX12/DX12GFXIndexBuffer.h>
#include <Diotima/Driver/DX12/DX12GFXVertexBuffer.h>

#include <Utils/Conversions.h>
#include <Utils/DebugUtils/Debug.h>
#include <Utils/Platform/FilePath.h>

#include <Optick/optick.h>

namespace Diotima
{

	DepthPass::DepthPass()
		: mDevice(nullptr)
	{
	}

	DepthPass::~DepthPass()
	{
	}

	void DepthPass::Initialize(int width, int height)
	{
		OPTICK_EVENT();

		//SetDevice(mRenderer->mDX12Device.get());

		CreateRootSignature();
		CreatePipelineState();

		mViewport = new D3D12_VIEWPORT();
		mViewport->Width = static_cast<float>(width);
		mViewport->Height = static_cast<float>(height);
		mViewport->TopLeftX = 0;
		mViewport->TopLeftY = 0;
		mViewport->MinDepth = 0.0f;
		mViewport->MaxDepth = 1.0f;

		mScissorRect.left = 0;
		mScissorRect.top = 0;
		mScissorRect.right = width;
		mScissorRect.bottom = height;

		mDepthStencilBuffer = std::make_unique<DX12GFXDepthStencilBuffer>();
		mDepthStencilBuffer->SetDevice(mDevice);
		mDepthStencilBuffer->SetSize(width, height);
		mDepthStencilBuffer->Allocate();

		mCommandList = mDevice->CreateGraphicsCommandList(mDevice->GetCommandAllocator(), nullptr);
		ID3D12GraphicsCommandList* commandList = mDevice->GetGraphicsCommandList(mCommandList);

		mDevice->GetDevice()->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32_TYPELESS, width, height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			nullptr,
			IID_PPV_ARGS(&mDepthTexture));

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;

		mDepthTexGPUHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mDevice->GetTextureHeap()->GetGPUDescriptorHandleForHeapStart());
		mDepthTexGPUHandle.Offset(600, mDevice->GetCBVSRVUAVDescriptorSize());

		mDepthTexCPUHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mDevice->GetTextureHeap()->GetCPUDescriptorHandleForHeapStart());
		mDepthTexCPUHandle.Offset(600, mDevice->GetCBVSRVUAVDescriptorSize());

		mDevice->GetDevice()->CreateShaderResourceView(mDepthTexture.Get(), &srvDesc, mDepthTexCPUHandle);

		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
			mDepthStencilBuffer->GetResource(),
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			D3D12_RESOURCE_STATE_RESOLVE_SOURCE));

		mDevice->ExecuteCommandList(commandList);

		mLightConstantBuffer = mDevice->CreateConstantBuffer(sizeof(Matrix4x4), 1);
		mWorldMatrixBuffer = mDevice->CreateConstantBuffer(sizeof(Matrix4x4), 1);
	}

	void DepthPass::Begin(ID3D12GraphicsCommandList* commandList)
	{
		commandList->RSSetViewports(1, mViewport);
		commandList->RSSetScissorRects(1, &mScissorRect);

		commandList->SetGraphicsRootSignature(mRootSignature.Get());

		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
			mDepthStencilBuffer->GetResource(),
			D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
			D3D12_RESOURCE_STATE_DEPTH_WRITE));

		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(mDepthStencilBuffer->GetDescriptorHeap()->GetCPUDescriptorHandleForHeapStart());
		commandList->OMSetRenderTargets(0, nullptr, FALSE, &dsvHandle);
		commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	}

	void DepthPass::Execute()
	{
		OPTICK_EVENT();

		ID3D12GraphicsCommandList* commandList = mDevice->GetGraphicsCommandList(mCommandList);
		commandList->Reset(mDevice->GetCommandAllocator(), mPipelineState.Get());

		Begin(commandList);
		{
			const std::vector<Renderer::LightItemProtocol>& lights = mRenderer->GetLights();
			PrepareLights(lights);

			DX12GFXConstantBuffer* const lightConstantBuffer = mDevice->GetConstantBuffer(mLightConstantBuffer);
			DX12GFXConstantBuffer* const worldMatrixBuffer = mDevice->GetConstantBuffer(mWorldMatrixBuffer);
			
			commandList->SetGraphicsRootConstantBufferView(0, lightConstantBuffer->GetResource()->GetGPUVirtualAddress());

			const std::vector<Renderer::RenderItemDrawCall>& drawCalls = mRenderer->GetDrawCalls();
			for (const Renderer::RenderItemDrawCall& drawCall : drawCalls)
			{
				//commandList->SetGraphicsRootConstantBufferView(1, drawCall.MatrixSlot.GPUBaseAddr);

				DX12GFXVertexBuffer* const vertexBuffer = mDevice->GetVertexBuffer(drawCall.VertexBuffer);
				DX12GFXIndexBuffer* const indexBuffer = mDevice->GetIndexBuffer(drawCall.IndexBuffer);

				commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				commandList->IASetVertexBuffers(0, 1, vertexBuffer->GetBufferView());
				commandList->IASetIndexBuffer(indexBuffer->GetBufferView());
				commandList->DrawIndexedInstanced(indexBuffer->GetNumElements(), 1, 0, 0, 0);
			}
		}
		End(commandList);
	}

	void DepthPass::End(ID3D12GraphicsCommandList* commandList)
	{
		D3D12_RESOURCE_BARRIER barriers[2] =
		{
			CD3DX12_RESOURCE_BARRIER::Transition(
				mDepthStencilBuffer->GetResource(),
				D3D12_RESOURCE_STATE_DEPTH_WRITE,
				D3D12_RESOURCE_STATE_RESOLVE_SOURCE),
			CD3DX12_RESOURCE_BARRIER::Transition(
				mDepthTexture.Get(),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_RESOLVE_DEST)
		};

		commandList->ResourceBarrier(2, barriers);

		commandList->ResolveSubresource(mDepthTexture.Get(), 0, mDepthStencilBuffer->GetResource(), 0, DXGI_FORMAT_R32_FLOAT);

		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
			mDepthTexture.Get(),
			D3D12_RESOURCE_STATE_RESOLVE_DEST,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

		mDevice->ExecuteCommandList(commandList);
	}

	void DepthPass::SetRenderer(Renderer* renderer)
	{
		mRenderer = renderer;
	}

	void DepthPass::SetDevice(DX12GFXDevice* device)
	{
		AssertNotNull(device);
		AssertExpr(mDevice == nullptr);

		mDevice = device;
	}

	void DepthPass::CreateRootSignature()
	{
		D3D12_ROOT_DESCRIPTOR1 worldMatrixbuffer;
		worldMatrixbuffer.RegisterSpace = 0;
		worldMatrixbuffer.ShaderRegister = 0;
		worldMatrixbuffer.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;

		D3D12_ROOT_DESCRIPTOR1 lightSpaceBuffer;
		lightSpaceBuffer.RegisterSpace = 1;
		lightSpaceBuffer.ShaderRegister = 0;
		lightSpaceBuffer.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;

		CD3DX12_ROOT_PARAMETER1 rootParameters[2];
		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[0].Descriptor = worldMatrixbuffer;
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

		rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[1].Descriptor = lightSpaceBuffer;
		rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init_1_1(_countof(rootParameters), &rootParameters[0], 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;

		D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_1, &signature, &error);
		if (error != nullptr)
		{
			OutputDebugStringA((char*)error->GetBufferPointer());
		}

		HRESULT res = mDevice->GetDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&mRootSignature));
	}

	void DepthPass::CreatePipelineState()
	{
		ComPtr<ID3DBlob> vertexShader;
		ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
		U32 compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		U32 compileFlags = 0;
#endif

		FilePath vertexShaderFilePath("Assets/Shaders/Shadows_VS.hlsl");
		FilePath pixelShaderFilePath("Assets/Shaders/Shadows_PS.hlsl");

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
		//psoDesc.PS = { reinterpret_cast<U8*>(pixelShader->GetBufferPointer()), pixelShader->GetBufferSize() };
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
		psoDesc.SampleDesc.Count = mDevice->GetMSAASampleCount();
		mDevice->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPipelineState));
	}

	void DepthPass::PrepareLights(const std::vector<Renderer::LightItemProtocol>& lights)
	{
		//OPTICK_EVENT();

		DX12GFXConstantBuffer* const lightConstantBuffer = mDevice->GetConstantBuffer(mLightConstantBuffer);

		lightConstantBuffer->Reset();

		AssertMsg(lights.size() == 1, "First pass shadows only support one light"); 
		lightConstantBuffer->AllocateMember(&lights[0].LightSpaceMatrix);
	}

	CD3DX12_GPU_DESCRIPTOR_HANDLE DepthPass::GetResourceGPUHandle()
	{
		mDepthTexGPUHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mDevice->GetTextureHeap()->GetGPUDescriptorHandleForHeapStart());
		mDepthTexGPUHandle.Offset(600, mDevice->GetCBVSRVUAVDescriptorSize());

		return mDepthTexGPUHandle;
	}

}

