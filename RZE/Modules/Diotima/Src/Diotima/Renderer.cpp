#include <Diotima/Renderer.h>

#include <Optick/optick.h>

#include <Diotima/Graphics/GFXPassGraph.h>

#include <Perseus/JobSystem/JobScheduler.h>

#include <Utils/Conversions.h>
#include <Utils/MemoryUtils.h>
#include <Utils/DebugUtils/Debug.h>
#include <Utils/Math/Vector4D.h>
#include <Utils/Platform/FilePath.h>

// DX11
#include <Diotima/Driver/DX11/DX11GFXDevice.h>
#include <Diotima/Driver/DX11/DX11.h>
// #TODO(What makes this needed here? unique_ptr complains re: destructor visibility
//       is it necessary otherwise?)
#include <Diotima/Driver/DX11/DX11GFXVertexBuffer.h>
#include <Diotima/Driver/DX11/DX11GFXIndexBuffer.h>
#include <Diotima/Driver/DX11/DX11GFXConstantBuffer.h>
#include <Diotima/Driver/DX11/DX11GFXTextureBuffer2D.h>

#include <ImGui/imgui.h>

#include <array>

namespace Diotima
{
	D3D11_INPUT_ELEMENT_DESC k_vertLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	UINT numLayoutElements = ARRAYSIZE(k_vertLayout);

	Renderer::Renderer()
		: mPassGraph(std::make_unique<GFXPassGraph>())
	{
		mLightingList.reserve(MAX_LIGHTS);
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

			mRenderItems[index] = std::move(itemProtocol);
			mRenderItems[index].bIsValid = true;
			mRenderItems[index].ConstantBuffer = mDevice->CreateConstantBuffer(sizeof(Matrix4x4), 1);

			return index;
		}

		mRenderItems.emplace_back(std::move(itemProtocol));
		mRenderItems.back().bIsValid = true;
		mRenderItems.back().ConstantBuffer = mDevice->CreateConstantBuffer(sizeof(Matrix4x4), 1);

		return static_cast<Int32>(mRenderItems.size() - 1);
	}

	void Renderer::RemoveRenderItem(const U32 itemIdx)
	{
		AssertExpr(itemIdx < mRenderItems.size());

		mRenderItems[itemIdx].Invalidate();
		mFreeRenderListIndices.push(itemIdx);
	}

	Int32 Renderer::AddLightItem(const LightItemProtocol& itemProtocol)
	{
		mLightingList.emplace_back(std::move(itemProtocol));

		++mLightCounts[itemProtocol.LightType];
		return static_cast<Int32>(mLightingList.size() - 1);
	}

	void Renderer::RemoveLightItem(const U32 itemIdx)
	{
		AssertExpr(itemIdx < mLightingList.size());

		auto iter = mLightingList.begin() + itemIdx;
		--mLightCounts[iter->LightType];
		mLightingList.erase(iter);
	}

	void Renderer::Initialize()
	{
		mCanvasSize.SetXY(1600, 900);

		mDevice = std::make_unique<DX11GFXDevice>();
		mDevice->SetWindow(mWindowHandle);
		mDevice->Initialize();

		mViewProjBuf = mDevice->CreateConstantBuffer(sizeof(Matrix4x4), 1);

		HRESULT hr;
		hr = mDevice->GetHardwareDevice().CreateInputLayout(k_vertLayout, numLayoutElements, mDevice->mVSBlob->GetBufferPointer(), mDevice->mVSBlob->GetBufferSize(), &mVertexLayout);
		//mPassGraph->Build(this);
	}

	void Renderer::Update()
	{
		OPTICK_EVENT();

		ProcessCommands();

		PrepareDrawCalls();

		{
			// #TODO(Eventually this should be moved out of here and into the engine level.
			//       Then the render side just deals with the generated commands.)
			//mPassGraph->Execute();
		}

// 		HRESULT res = mDevice->GetDevice()->GetDeviceRemovedReason();
// 		if (res != S_OK)
// 		{
// 			AssertFalse();
// 		}

		FLOAT rgba[4] = { 0.5f, 0.5f, 1.0f, 1.0f };
		ID3D11DeviceContext& deviceContext = mDevice->GetDeviceContext();
		deviceContext.ClearDepthStencilView(mDevice->mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
		deviceContext.ClearRenderTargetView(mDevice->mRenderTargetView, rgba);

		DX11GFXConstantBuffer* viewProjBuf = mDevice->GetConstantBuffer(mViewProjBuf);
		ID3D11Buffer* vpbHardwareBuf = &viewProjBuf->GetHardwareBuffer();
		mDevice->GetDeviceContext().VSSetConstantBuffers(0, 1, &vpbHardwareBuf);
		for (RenderItemDrawCall& drawCall : mPerFrameDrawCalls)
		{
			// try to draw something
			mDevice->GetDeviceContext().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			DX11GFXConstantBuffer* modelMatBuf = mDevice->GetConstantBuffer(drawCall.ConstantBuffer);
			ID3D11Buffer* hwModelMatBuf = &modelMatBuf->GetHardwareBuffer();
			deviceContext.VSSetConstantBuffers(1, 1, &hwModelMatBuf);

			ID3D11Buffer* vertBuf = &mDevice->GetVertexBuffer(drawCall.VertexBuffer)->GetHardwareBuffer();
			
			struct TempDataLayoutStructure
			{
				Vector3D position;
 				Vector2D uv;
// 				Vector3D normal;
// 				Vector3D tangents;
			};
			UINT stride = sizeof(TempDataLayoutStructure);
			UINT offset = 0;
			mDevice->GetDeviceContext().IASetVertexBuffers(0, 1, &vertBuf, &stride, &offset);

			mDevice->GetDeviceContext().IASetInputLayout(mVertexLayout);

			// Index buffer
			DX11GFXIndexBuffer* indexBuf = mDevice->GetIndexBuffer(drawCall.IndexBuffer);
			mDevice->GetDeviceContext().IASetIndexBuffer(&indexBuf->GetHardwareBuffer(), DXGI_FORMAT_R32_UINT, 0);

			std::array<DX11GFXTextureBuffer2D*, 3> textureArray = {
				mDevice->GetTextureBuffer2D(drawCall.TextureSlot0),
				mDevice->GetTextureBuffer2D(drawCall.TextureSlot1),
				mDevice->GetTextureBuffer2D(drawCall.TextureSlot2)
			};

			for (size_t texBufIdx = 0; texBufIdx < textureArray.size(); ++texBufIdx)
			{
				ID3D11ShaderResourceView* resourceView = &textureArray[texBufIdx]->GetResourceView();
				ID3D11SamplerState* samplerState = &textureArray[texBufIdx]->GetSamplerState();
				deviceContext.PSSetShaderResources(texBufIdx, 1, &resourceView);
				deviceContext.PSSetSamplers(texBufIdx, 1, &samplerState);
			}

			deviceContext.DrawIndexed(indexBuf->GetIndexCount(), 0, 0);
		}
	}

	void Renderer::Render()
	{
		mDevice->Present();
	}

	void Renderer::ShutDown()
	{
		//ImGui_ImplDX12_Shutdown();

		mDevice->Shutdown();
	}

	const std::vector<Renderer::RenderItemDrawCall>& Renderer::GetDrawCalls()
	{
		return mPerFrameDrawCalls;
	}

	const std::vector<Renderer::LightItemProtocol>& Renderer::GetLights()
	{
		return mLightingList;
	}

	const U32* Renderer::GetLightCounts()
	{
		return mLightCounts;
	}

	const Diotima::Renderer::CameraItemProtocol& Renderer::GetCamera()
	{
		return camera;
	}

	void Renderer::PrepareDrawCalls()
	{
		OPTICK_EVENT();

		if (mPerFrameDrawCalls.capacity() < mRenderItems.size())
		{
			mPerFrameDrawCalls.reserve(mRenderItems.size());
		}

		Matrix4x4 camViewProjMat = camera.ProjectionMat * camera.ViewMat;
		DX11GFXConstantBuffer* viewProjBuf = mDevice->GetConstantBuffer(mViewProjBuf);
		viewProjBuf->UpdateSubresources(&camViewProjMat);

		mPerFrameDrawCalls.clear();
		for (RenderItemProtocol& renderItem : mRenderItems)
		{
			// #TODO(Josh::This needs to be removed -- an opaque handle should be leased out that will
			//             get fixed up when we remove render items)
			if (!renderItem.bIsValid)
			{
				continue;
			}

			DX11GFXConstantBuffer* constBuf = mDevice->GetConstantBuffer(renderItem.ConstantBuffer);
			constBuf->UpdateSubresources(&renderItem.ModelMatrix);

			for (RenderItemMeshData& meshData : renderItem.MeshData)
			{
				RenderItemDrawCall drawCall;
				drawCall.VertexBuffer = meshData.VertexBuffer;
				drawCall.IndexBuffer = meshData.IndexBuffer;
				// #TODO(Deal with this later. This is the same for all meshes of renderItem)
				drawCall.ConstantBuffer = renderItem.ConstantBuffer;

				// #TODO(Josh::Eventually the material system will handle this itself and hold a buffer of pre-allocated materials
				//             and we will somehow "lease" it for the draw call)
// 				drawCall.MaterialSlot = materialBuffer->AllocateMember(&meshData.Material);
// 
 				drawCall.TextureSlot0 = meshData.TextureDescs[0].TextureBuffer; // This should be iterated on to be more robust.
 				drawCall.TextureSlot1 = meshData.TextureDescs[1].TextureBuffer; // This should be iterated on to be more robust.
 				drawCall.TextureSlot2 = meshData.TextureDescs[2].TextureBuffer; // This should be iterated on to be more robust.

				//drawCall.MatrixSlot = matrixSlot;

				mPerFrameDrawCalls.push_back(std::move(drawCall));
			}
		}

		//memset(mMatrixConstantBuffer, 0, sizeof(Matrix4x4) * 3);
	}

	void Renderer::EnableVsync(bool bEnabled)
	{
		//mDevice->SetSyncInterval(static_cast<U32>(bEnabled));
	}

	void Renderer::SetMSAASampleCount(U32 sampleCount)
	{
		mMSAASampleCount = sampleCount;
	}
	
	const Vector2D& Renderer::GetCanvasSize()
	{
		return mCanvasSize;
	}

	void Renderer::ResizeCanvas(const Vector2D& newSize)
	{
		mCanvasSize = newSize;

		int width = static_cast<int>(newSize.X());
		int height = static_cast<int>(newSize.Y());

		//mDX12Device->HandleWindowResize(width, height);

		mPassGraph->OnWindowResize(width, height);

		LOG_CONSOLE_ARGS("New Canvas Size: %f x %f", mCanvasSize.X(), mCanvasSize.Y());
	}

	U32 Renderer::CreateVertexBuffer(void* data, size_t size, U32 count)
	{
		return mDevice->CreateVertexBuffer(data, size, count);
	}

	U32 Renderer::CreateIndexBuffer(void* data, size_t size, U32 count)
	{
		return mDevice->CreateIndexBuffer(data, size, count);
	}

	U32 Renderer::CreateTextureBuffer2D(void* data, U32 width, U32 height)
	{
		return mDevice->CreateTextureBuffer2D(data, width, height);
	}

	U32 Renderer::QueueCreateVertexBufferCommand(void* data, size_t size, U32 count)
	{
		std::lock_guard<std::mutex> lock(mVertexBufferCommandMutex);

		CreateBufferRenderCommand command;
		command.BufferType = ECreateBufferType::Vertex;
		command.Data = data;
		command.Size = size;
		command.Count = count;
		mVertexBufferCommandQueue.push_back(std::move(command));

		// #TODO(This is shit, fix later)
		return mDevice->GetVertexBufferCount() + mVertexBufferCommandQueue.size() - 1;
	}

	U32 Renderer::QueueCreateIndexBufferCommand(void* data, size_t size, U32 count)
	{
		std::lock_guard<std::mutex> lock(mIndexBufferCommandMutex);

		CreateBufferRenderCommand command;
		command.BufferType = ECreateBufferType::Index;
		command.Data = data;
		command.Size = size;
		command.Count = count;
		mIndexBufferCommandQueue.push_back(std::move(command));

		// #TODO(This is shit, fix later)
		return mDevice->GetVertexBufferCount() + mIndexBufferCommandQueue.size() - 1;
	}

	U32 Renderer::QueueCreateTextureCommand(ECreateTextureBufferType bufferType, void* data, U32 width, U32 height)
	{
		std::lock_guard<std::mutex> lock(mTextureBufferCommandMutex);

		CreateTextureBufferRenderCommand command;
		command.BufferType = bufferType;
		command.Data = data;
		command.Width = width;
		command.Height = height;
		mTextureBufferCommandQueue.push_back(std::move(command));

		// #TODO(This is shit, fix later)
		return mDevice->GetTextureBufferCount() + mTextureBufferCommandQueue.size() - 1;
	}


	void Renderer::QueueUpdateRenderItem(U32 itemID, const Matrix4x4& worldMtx)
	{
		std::lock_guard<std::mutex> lock(mUpdateRenderItemWorldMatrixCommandMutex);

		UpdateRenderItemWorldMatrixCommand command;
		command.RenderItemID = itemID;
		command.WorldMtx = worldMtx;

		mUpdateRenderItemWorldMatrixCommandQueue.push_back(std::move(command));
	}

	void Renderer::ProcessCommands()
	{
		OPTICK_EVENT();

		{
			OPTICK_EVENT("Process CreateVertexBuffer commands");
			std::lock_guard<std::mutex> lock(mVertexBufferCommandMutex);
			for (auto& command : mVertexBufferCommandQueue)
			{
				CreateVertexBuffer(command.Data, command.Size, command.Count);
			}
			mVertexBufferCommandQueue.clear();
		}

		{
			OPTICK_EVENT("Process CreateIndexBuffer commands");
			std::lock_guard<std::mutex> lock(mIndexBufferCommandMutex);
			for (auto& command : mIndexBufferCommandQueue)
			{
				CreateIndexBuffer(command.Data, command.Size, command.Count);
			}
			mIndexBufferCommandQueue.clear();
		}

		{
			OPTICK_EVENT("Process CreateTextureBuffer commands");
			std::lock_guard<std::mutex> lock(mTextureBufferCommandMutex);
			for (auto& command : mTextureBufferCommandQueue)
			{
				switch (command.BufferType)
				{
				case ECreateTextureBufferType::Texture2D:
					CreateTextureBuffer2D(command.Data, command.Width, command.Height);
					break;

				default:
					AssertFalse();
					break;
				}
			}
			mTextureBufferCommandQueue.clear();
		}

		{
			OPTICK_EVENT("Process UpdateRenderItemWorldMatrix commands");
			std::lock_guard<std::mutex> lock(mUpdateRenderItemWorldMatrixCommandMutex);
			for (auto& command : mUpdateRenderItemWorldMatrixCommandQueue)
			{
				RenderItemProtocol& renderItem = mRenderItems[command.RenderItemID];
				AssertExpr(renderItem.bIsValid);

				renderItem.ModelMatrix = command.WorldMtx;
			}
			mUpdateRenderItemWorldMatrixCommandQueue.clear();
		}
	}

	void Renderer::RenderItemProtocol::Invalidate()
	{
		bIsValid = false;
	}

}
