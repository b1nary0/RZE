#include <Diotima/Graphics/RenderPasses/ForwardPass.h>

#include <Diotima/Driver/DX12/DX12GFXDevice.h>
#include <Diotima/Driver/DX12/DX12GFXConstantBuffer.h>
#include <Diotima/Driver/DX12/DX12GFXDepthStencilBuffer.h>
#include <Diotima/Driver/DX12/DX12GFXTextureBuffer2D.h>
#include <Diotima/Driver/DX12/DX12GFXIndexBuffer.h>
#include <Diotima/Driver/DX12/DX12GFXVertexBuffer.h>

#include <Utils/Conversions.h>
#include <Utils/DebugUtils/Debug.h>
#include <Utils/Platform/FilePath.h>

#include <Optick/optick.h>

namespace Diotima
{

	ForwardPass::ForwardPass()
		: mDevice(nullptr)
	{
	}

	ForwardPass::~ForwardPass()
	{
	}

	void ForwardPass::Initialize(int width, int height)
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
 
 		//WaitForPreviousFrame();
 
 		mCommandList = mDevice->CreateGraphicsCommandList(mDevice->GetCommandAllocator(), nullptr);
 		mDevice->GetGraphicsCommandList(mCommandList)->Close();
 
 		mLightConstantBuffer = mDevice->CreateConstantBuffer(sizeof(Renderer::LightItemProtocol) * MAX_LIGHTS, 1);
 		mPerFramePixelShaderConstants = mDevice->CreateConstantBuffer(sizeof(U32) * 2, 1);
	}

	void ForwardPass::Begin(ID3D12GraphicsCommandList* commandList)
	{
		commandList->RSSetViewports(1, mViewport);
		commandList->RSSetScissorRects(1, &mScissorRect);

		commandList->SetGraphicsRootSignature(mRootSignature.Get());

		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDevice->GetMSAARenderTarget(), D3D12_RESOURCE_STATE_RESOLVE_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(mDevice->GetMSAARTVHeap()->GetCPUDescriptorHandleForHeapStart(), 0, mDevice->GetRTVDescriptorSize());
		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(mDepthStencilBuffer->GetDescriptorHeap()->GetCPUDescriptorHandleForHeapStart());
		commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
		commandList->ClearDepthStencilView(mDepthStencilBuffer->GetDescriptorHeap()->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		const float clearColor[] = { 0.f, 0.f, 0.f, 0.0f };
		commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	}

	void ForwardPass::Execute()
	{
		OPTICK_EVENT();

		ID3D12GraphicsCommandList* commandList = mDevice->GetGraphicsCommandList(mCommandList);
		commandList->Reset(mDevice->GetCommandAllocator(), mPipelineState.Get());

		Begin(commandList);
		{
			const Renderer::CameraItemProtocol& camera = mRenderer->GetCamera();

			const std::vector<Renderer::LightItemProtocol>& lights = mRenderer->GetLights();
			const U32* lightCounts = mRenderer->GetLightCounts();
			PrepareLights(lights, lightCounts);

			CBAllocationData lightBufferStart = mDevice->GetConstantBuffer(mLightConstantBuffer)->GetBufferStart();
			CBAllocationData perFrameShaderBufferStart = mDevice->GetConstantBuffer(mPerFramePixelShaderConstants)->GetBufferStart();

			ID3D12DescriptorHeap* ppDescHeaps[] = { mDevice->GetTextureHeap() };
			commandList->SetDescriptorHeaps(_countof(ppDescHeaps), ppDescHeaps);

			commandList->SetGraphicsRootConstantBufferView(2, lightBufferStart.GPUBaseAddr);
			commandList->SetGraphicsRootConstantBufferView(5, perFrameShaderBufferStart.GPUBaseAddr);

			commandList->SetGraphicsRoot32BitConstants(1, 3, &camera.Position.GetInternalVec(), 0);

			commandList->SetGraphicsRootDescriptorTable(8, mInputResourceHandle);

			const std::vector<Renderer::RenderItemDrawCall>& drawCalls = mRenderer->GetDrawCalls();
			for (const Renderer::RenderItemDrawCall& drawCall : drawCalls)
			{
// 				commandList->SetGraphicsRootConstantBufferView(0, drawCall.MatrixSlot.GPUBaseAddr);
// 				commandList->SetGraphicsRootConstantBufferView(4, drawCall.MaterialSlot.GPUBaseAddr);

				// #NOTE(Josh::Everything should have a default guaranteed diffuse map. For now it also marks the start of the descriptor table)
// 				DX12GFXTextureBuffer2D* const diffuse = mDevice->GetTextureBuffer2D(drawCall.TextureSlot0);
// 				DX12GFXTextureBuffer2D* const specular = mDevice->GetTextureBuffer2D(drawCall.TextureSlot1);
// 				DX12GFXTextureBuffer2D* const bump = mDevice->GetTextureBuffer2D(drawCall.TextureSlot2);

// 				commandList->SetGraphicsRootDescriptorTable(3, diffuse->GetDescriptorHandleGPU());
// 				commandList->SetGraphicsRootDescriptorTable(6, specular->GetDescriptorHandleGPU());
// 				commandList->SetGraphicsRootDescriptorTable(7, bump->GetDescriptorHandleGPU());

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

	void ForwardPass::End(ID3D12GraphicsCommandList* commandList)
	{
		ID3D12Resource* currentFrameRenderTarget = mDevice->GetCurrentFrameRenderTarget();

		D3D12_RESOURCE_BARRIER barriers[2] =
		{
			CD3DX12_RESOURCE_BARRIER::Transition(
				mDevice->GetMSAARenderTarget(),
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_RESOLVE_SOURCE),
			CD3DX12_RESOURCE_BARRIER::Transition(
				currentFrameRenderTarget,
				D3D12_RESOURCE_STATE_PRESENT,
				D3D12_RESOURCE_STATE_RESOLVE_DEST)
		};

		commandList->ResourceBarrier(2, barriers);

		commandList->ResolveSubresource(currentFrameRenderTarget, 0, mDevice->GetMSAARenderTarget(), 0, DXGI_FORMAT_R8G8B8A8_UNORM);

		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
			currentFrameRenderTarget,
			D3D12_RESOURCE_STATE_RESOLVE_DEST,
			D3D12_RESOURCE_STATE_PRESENT));

		mDevice->ExecuteCommandList(commandList);
	}

	void ForwardPass::SetResourceGPUHandle(CD3DX12_GPU_DESCRIPTOR_HANDLE handle)
	{
		mInputResourceHandle = handle;
	}

	void ForwardPass::SetRenderer(Renderer* renderer)
	{
		mRenderer = renderer;
	}

	void ForwardPass::SetDevice(DX12GFXDevice* device)
	{
		AssertNotNull(device);
		AssertExpr(mDevice == nullptr);

		mDevice = device;
	}

	void ForwardPass::CreateRootSignature()
	{
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

		D3D12_DESCRIPTOR_RANGE1 diffuseRange[1];
		diffuseRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		diffuseRange[0].NumDescriptors = 1;
		diffuseRange[0].BaseShaderRegister = 0;
		diffuseRange[0].RegisterSpace = 0;
		diffuseRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		diffuseRange[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

		D3D12_ROOT_DESCRIPTOR_TABLE1 diffuseTable;
		diffuseTable.NumDescriptorRanges = _countof(diffuseRange);
		diffuseTable.pDescriptorRanges = &diffuseRange[0];

		D3D12_DESCRIPTOR_RANGE1 specularRange[1];
		specularRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		specularRange[0].NumDescriptors = 1;
		specularRange[0].BaseShaderRegister = 1;
		specularRange[0].RegisterSpace = 0;
		specularRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		specularRange[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

		D3D12_ROOT_DESCRIPTOR_TABLE1 specularTable;
		specularTable.NumDescriptorRanges = _countof(specularRange);
		specularTable.pDescriptorRanges = &specularRange[0];

		D3D12_DESCRIPTOR_RANGE1 bumpRange[1];
		bumpRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		bumpRange[0].NumDescriptors = 1;
		bumpRange[0].BaseShaderRegister = 2;
		bumpRange[0].RegisterSpace = 0;
		bumpRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		bumpRange[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

		D3D12_ROOT_DESCRIPTOR_TABLE1 bumpTable;
		bumpTable.NumDescriptorRanges = _countof(bumpRange);
		bumpTable.pDescriptorRanges = &bumpRange[0];

		D3D12_DESCRIPTOR_RANGE1 shadowsRange[1];
		shadowsRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		shadowsRange[0].NumDescriptors = 1;
		shadowsRange[0].BaseShaderRegister = 3;
		shadowsRange[0].RegisterSpace = 0;
		shadowsRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		shadowsRange[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;

		D3D12_ROOT_DESCRIPTOR_TABLE1 shadowsTable;
		shadowsTable.NumDescriptorRanges = _countof(shadowsRange);
		shadowsTable.pDescriptorRanges = &shadowsRange[0];

		CD3DX12_ROOT_PARAMETER1 rootParameters[9];
		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[0].Descriptor = mMVPConstBuffer;
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

		rootParameters[1].InitAsConstants(3, 0, 2, D3D12_SHADER_VISIBILITY_PIXEL);

		rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[2].Descriptor = mLightConstBuffer;
		rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[3].DescriptorTable = diffuseTable;
		rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		rootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[4].Descriptor = mPerMeshPixelShaderConstants;
		rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		rootParameters[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[5].Descriptor = mPerFramePixelShaderConstants;
		rootParameters[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		rootParameters[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[6].DescriptorTable = specularTable;
		rootParameters[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		rootParameters[7].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[7].DescriptorTable = bumpTable;
		rootParameters[7].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		rootParameters[8].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[8].DescriptorTable = shadowsTable;
		rootParameters[8].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_STATIC_SAMPLER_DESC sampler = {};
		sampler.Filter = D3D12_FILTER_ANISOTROPIC;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.MipLODBias = 0;
		sampler.MaxAnisotropy = 16;
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

		HRESULT res = mDevice->GetDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&mRootSignature));
	}

	void ForwardPass::CreatePipelineState()
	{
		ComPtr<ID3DBlob> vertexShader;
		ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
		U32 compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		U32 compileFlags = 0;
#endif

		FilePath vertexShaderFilePath("Assets/Shaders/ForwardLighting_VS.hlsl");
		FilePath pixelShaderFilePath("Assets/Shaders/ForwardLighting_PS.hlsl");

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
		psoDesc.SampleDesc.Count = mDevice->GetMSAASampleCount();
		mDevice->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPipelineState));
	}

	void ForwardPass::PrepareLights(const std::vector<Renderer::LightItemProtocol>& lights, const U32* lightCounts)
	{
		//OPTICK_EVENT();

		DX12GFXConstantBuffer* const lightConstantBuffer = mDevice->GetConstantBuffer(mLightConstantBuffer);
		DX12GFXConstantBuffer* const perFramePixelShaderConstants = mDevice->GetConstantBuffer(mPerFramePixelShaderConstants);

		lightConstantBuffer->Reset();
		perFramePixelShaderConstants->Reset();

		lightConstantBuffer->AllocateMember(lights.data());
		perFramePixelShaderConstants->AllocateMember(lightCounts);
	}

	void ForwardPass::OnWindowResize(int newWidth, int newHeight)
	{
		mDepthStencilBuffer.release();

		mDepthStencilBuffer = std::make_unique<DX12GFXDepthStencilBuffer>();
		mDepthStencilBuffer->SetSize(newWidth, newHeight);
		mDepthStencilBuffer->SetDevice(mDevice);
		mDepthStencilBuffer->Allocate();

		mViewport->Width = static_cast<float>(newWidth);
		mViewport->Height = static_cast<float>(newHeight);
		mViewport->TopLeftX = 0;
		mViewport->TopLeftY = 0;
		mViewport->MinDepth = 0.0f;
		mViewport->MaxDepth = 1.0f;

		mScissorRect.left = 0;
		mScissorRect.top = 0;
		mScissorRect.right = newWidth;
		mScissorRect.bottom = newHeight;
	}

}

