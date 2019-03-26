#include <Diotima/Renderer.h>

#include <Brofiler/Brofiler.h>

#include <Utils/Conversions.h>
#include <Utils/MemoryUtils.h>
#include <Utils/DebugUtils/Debug.h>
#include <Utils/Math/Vector4D.h>
#include <Utils/Platform/FilePath.h>

// DX12 Branch Temp
#include <Diotima/Driver/DX12/DX12GFXDevice.h>
#include <Diotima/Driver/DX12/DX12GFXConstantBuffer.h>
#include <Diotima/Driver/DX12/DX12GFXIndexBuffer.h>
#include <Diotima/Driver/DX12/DX12GFXVertexBuffer.h>
#include <Diotima/Driver/DX12/DX12GFXTextureBuffer2D.h>

namespace Diotima
{
	Renderer::Renderer()
	{
	}

	Renderer::~Renderer()
	{
	}

	Int32 Renderer::AddRenderItem(const RenderItemProtocol& itemProtocol)
	{
		if (!mFreeRenderListIndices.empty())
		{
			Int32 index = mFreeRenderListIndices.front();
			mFreeRenderListIndices.pop();

			mRenderList[index] = std::move(itemProtocol);
			mRenderList[index].bIsValid = true;

			return index;
		}

		mRenderList.emplace_back(std::move(itemProtocol));
		mRenderList.back().bIsValid = true;

		return static_cast<Int32>(mRenderList.size() - 1);
	}

	void Renderer::RemoveRenderItem(const U32 itemIdx)
	{
		AssertExpr(itemIdx < mRenderList.size());

		mRenderList[itemIdx].Invalidate();
		mFreeRenderListIndices.push(itemIdx);
	}

	Int32 Renderer::AddLightItem(const LightItemProtocol& itemProtocol)
	{
		mLightingList.emplace_back(std::move(itemProtocol));
		return static_cast<Int32>(mLightingList.size() - 1);
	}

	void Renderer::Initialize()
	{
		DX12Initialize();
	}

	void Renderer::Update()
	{
		BROFILER_CATEGORY("Renderer::Update", Profiler::Color::Red);

		mDevice->ResetCommandAllocator();
		mDevice->ResetCommandList();

		PrepareLights();

		mDevice->BeginFrame();
		{
			ID3D12GraphicsCommandList* commandList = mDevice->GetCommandList();
			DX12GFXConstantBuffer* const MVPConstantBuffer = mDevice->GetConstantBuffer(mMVPConstantBuffer);
			DX12GFXConstantBuffer* const lightConstantBuffer = mDevice->GetConstantBuffer(mLightConstantBuffer);
			DX12GFXConstantBuffer* const perMeshPixelShaderConstants = mDevice->GetConstantBuffer(mPerMeshPixelShaderConstants);
			DX12GFXConstantBuffer* const perFramePixelShaderConstants = mDevice->GetConstantBuffer(mPerFramePixelShaderConstants);

			ID3D12DescriptorHeap* ppDescHeaps[] = { mDevice->GetTextureHeap() };
			commandList->SetDescriptorHeaps(_countof(ppDescHeaps), ppDescHeaps);

			commandList->SetGraphicsRootConstantBufferView(2, lightConstantBuffer->GetResource()->GetGPUVirtualAddress());
			commandList->SetGraphicsRootConstantBufferView(5, perFramePixelShaderConstants->GetResource()->GetGPUVirtualAddress());

			commandList->SetGraphicsRoot32BitConstants(1, 3, &camera.Position.GetInternalVec(), 0);

			Matrix4x4 camViewProjMat = camera.ProjectionMat * camera.ViewMat;

			void* pMatrixConstantBufferData = malloc(sizeof(Matrix4x4) * 3);
			U32 objectIndex = 0;

			// This will count the material instances to offset the constant buffer memory. Temporary.
			U32 materialIndex_HACK = 0;
			for (RenderItemProtocol& itemProtocol : mRenderList)
			{
				// #TODO(Josh::This needs to be removed -- an opaque handle should be leased out that will
				//             get fixed up when we remove render items)
				if (!itemProtocol.bIsValid)
				{
					continue;
				}

				const float* modelViewPtr = itemProtocol.ModelMatrix.GetValuePtr();
				const float* modelViewInvPtr = itemProtocol.ModelMatrix.Inverse().GetValuePtr();
				const float* camViewProjPtr = camViewProjMat.GetValuePtr();

				memcpy(pMatrixConstantBufferData, modelViewPtr, sizeof(Matrix4x4));
				memcpy((U8*)pMatrixConstantBufferData + sizeof(Matrix4x4), modelViewInvPtr, sizeof(Matrix4x4));
				memcpy((U8*)pMatrixConstantBufferData + sizeof(Matrix4x4) * 2, camViewProjPtr, sizeof(Matrix4x4));

				MVPConstantBuffer->SetData(pMatrixConstantBufferData, sizeof(Matrix4x4) * 3, objectIndex);

				{
					U32 alignedOffset = MemoryUtils::AlignSize(sizeof(Matrix4x4) * 3, 255) * objectIndex;
					commandList->SetGraphicsRootConstantBufferView(0, MVPConstantBuffer->GetResource()->GetGPUVirtualAddress() + alignedOffset);
				}
				
				for (size_t index = 0; index < itemProtocol.MeshData.size(); ++index)
				{
					const RenderItemMeshData& meshData = itemProtocol.MeshData[index];
					
					DX12GFXVertexBuffer* const vertexBuffer = mDevice->GetVertexBuffer(meshData.VertexBuffer);
					DX12GFXIndexBuffer* const indexBuffer = mDevice->GetIndexBuffer(meshData.IndexBuffer);

					// #TODO(Josh::This temp fix treats the offset as a 1D index for a 2D array where objectIndex : row and index : col
					//             Is a safe assumption for now since Renderer::Update is brute force atm, but the great cleanup will get to this)
					perMeshPixelShaderConstants->SetData(&meshData.Material, sizeof(RenderItemMaterialDesc), materialIndex_HACK);

					{
						U32 alignedOffset = MemoryUtils::AlignSize(sizeof(RenderItemMaterialDesc), 255) * materialIndex_HACK;
						commandList->SetGraphicsRootConstantBufferView(4, perMeshPixelShaderConstants->GetResource()->GetGPUVirtualAddress() + alignedOffset);
					}

					// #NOTE(Josh::Everything should have a default guaranteed diffuse map. For now it also marks the start of the descriptor table)
					DX12GFXTextureBuffer2D* const diffuseBuffer = mDevice->GetTextureBuffer2D(meshData.TextureDescs[0].TextureBuffer);
					commandList->SetGraphicsRootDescriptorTable(3, diffuseBuffer->GetDescriptorHandleGPU());

					commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
					commandList->IASetVertexBuffers(0, 1, vertexBuffer->GetBufferView());
					commandList->IASetIndexBuffer(indexBuffer->GetBufferView());
					commandList->DrawIndexedInstanced(indexBuffer->GetNumElements(), 1, 0, 0, 0);

					++materialIndex_HACK;
				}

				++objectIndex;
			}

			delete pMatrixConstantBufferData;
		}
		mDevice->EndFrame();
	}

	void Renderer::Render()
	{
		mDevice->Present();
	}

	void Renderer::ShutDown()
	{
		mDevice->Shutdown();
	}

	void Renderer::DX12Initialize()
	{
		mDevice = std::make_unique<DX12GFXDevice>();
		mDevice->SetWindow(mWindowHandle);
		mDevice->SetMSAASampleCount(mMSAASampleCount);
		mDevice->Initialize();

		mMVPConstantBuffer = mDevice->CreateConstantBuffer(sizeof(Matrix4x4) * 3, 64);
		mLightConstantBuffer = mDevice->CreateConstantBuffer(sizeof(LightItemProtocol), 64);
		mPerMeshPixelShaderConstants = mDevice->CreateConstantBuffer(sizeof(RenderItemMaterialDesc), 64);
		mPerFramePixelShaderConstants = mDevice->CreateConstantBuffer(sizeof(U32) * 2, 64);
	}

	void Renderer::PrepareLights()
	{
		std::vector<LightItemProtocol> directionalLights;
		std::vector<LightItemProtocol> pointLights;

		for (LightItemProtocol light : mLightingList)
		{
			if (light.LightType == ELightType::Directional)
			{
				directionalLights.push_back(light);
			}
			else
			{
				pointLights.push_back(light);
			}
		}

		DX12GFXConstantBuffer* const lightConstantBuffer = mDevice->GetConstantBuffer(mLightConstantBuffer);
		lightConstantBuffer->SetData(pointLights.data(), static_cast<U32>(sizeof(LightItemProtocol) * pointLights.size()), 0);
		//lightConstantBuffer->SetData(directionalLights.data(), static_cast<U32>(sizeof(LightItemProtocol) * directionalLights.size()), 1);

		DX12GFXConstantBuffer* const perFramePixelShaderConstants = mDevice->GetConstantBuffer(mPerFramePixelShaderConstants);
		U32 lightCounts[2] = { static_cast<U32>(pointLights.size()), static_cast<U32>(directionalLights.size()) };
		perFramePixelShaderConstants->SetData(lightCounts, sizeof(U32) * 2, 0);
	}

	void Renderer::EnableVsync(bool bEnabled)
	{
		
	}

	void Renderer::SetMSAASampleCount(U32 sampleCount)
	{
		mMSAASampleCount = sampleCount;
	}

	void Renderer::ResizeCanvas(const Vector2D& newSize)
	{
		mCanvasSize = newSize;

		LOG_CONSOLE_ARGS("New Canvas Size: %f x %f", mCanvasSize.X(), mCanvasSize.Y());
	}


	U32 Renderer::CreateVertexBuffer(void* data, U32 numElements)
	{
		return mDevice->CreateVertexBuffer(data, numElements);
	}


	U32 Renderer::CreateIndexBuffer(void* data, U32 numElements)
	{
		return mDevice->CreateIndexBuffer(data, numElements);
	}


	U32 Renderer::CreateTextureBuffer2D(void* data, U32 width, U32 height)
	{
		return mDevice->CreateTextureBuffer2D(data, width, height);
	}

	void Renderer::RenderItemProtocol::Invalidate()
	{
		bIsValid = false;
	}

}
