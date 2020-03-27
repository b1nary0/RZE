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
#include <ImGui/imgui_impl_dx11.h>

#include <array>

namespace Diotima
{
	D3D11_INPUT_ELEMENT_DESC k_vertLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 }
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
		U32 constantBufferID = mDevice->CreateConstantBuffer(sizeof(Matrix4x4), 2);

		std::vector<U32> meshMaterialBuffers;
		meshMaterialBuffers.reserve(itemProtocol.MeshData.size());
		for (size_t index = 0; index < itemProtocol.MeshData.size(); ++index)
		{
			meshMaterialBuffers.push_back(mDevice->CreateConstantBuffer(MemoryUtils::AlignSize(sizeof(RenderItemMaterialDesc), 15), 1));
		}

		if (!mFreeRenderListIndices.empty())
		{
			Int32 index = mFreeRenderListIndices.front();
			mFreeRenderListIndices.pop();

			mRenderItems[index] = std::move(itemProtocol);
			mRenderItems[index].bIsValid = true;
			mRenderItems[index].ConstantBuffer = constantBufferID;
			mRenderItems[index].MaterialBuffers = std::move(meshMaterialBuffers);

			return index;
		}

		mRenderItems.emplace_back(std::move(itemProtocol));
		mRenderItems.back().bIsValid = true;
		mRenderItems.back().ConstantBuffer = constantBufferID;
		mRenderItems.back().MaterialBuffers = std::move(meshMaterialBuffers);

		return static_cast<Int32>(mRenderItems.size() - 1);
	}

	void Renderer::RemoveRenderItem(const U32 itemIdx)
	{
		AssertExpr(itemIdx < mRenderItems.size());

		mRenderItems[itemIdx].Invalidate();
		mFreeRenderListIndices.push(itemIdx);

		// #TODO(Return constant buffer for render item here)
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

		ImGui::CreateContext();
		ImGui_ImplDX11_Init(&mDevice->GetHardwareDevice(), &mDevice->GetDeviceContext());

		mViewProjBuf = mDevice->CreateConstantBuffer(sizeof(Matrix4x4), 1);
		mLightBuf = mDevice->CreateConstantBuffer(sizeof(LightItemProtocol), 1);
		mCameraDataBuf = mDevice->CreateConstantBuffer(MemoryUtils::AlignSize(sizeof(Vector3D), 15), 1);

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

		FLOAT rgba[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		ID3D11DeviceContext& deviceContext = mDevice->GetDeviceContext();
		deviceContext.ClearDepthStencilView(mDevice->mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
		deviceContext.ClearRenderTargetView(mDevice->mRenderTargetView, rgba);

		deviceContext.RSSetState(mDevice->mRasterState);

		DX11GFXConstantBuffer* viewProjBuf = mDevice->GetConstantBuffer(mViewProjBuf);
		ID3D11Buffer* vpbHardwareBuf = &viewProjBuf->GetHardwareBuffer();
		deviceContext.VSSetConstantBuffers(0, 1, &vpbHardwareBuf);

		DX11GFXConstantBuffer* lightBuf = mDevice->GetConstantBuffer(mLightBuf);
		ID3D11Buffer* hwLightBuf = &lightBuf->GetHardwareBuffer();
		deviceContext.PSSetConstantBuffers(0, 1, &hwLightBuf);

		DX11GFXConstantBuffer* camDataBuf = mDevice->GetConstantBuffer(mCameraDataBuf);
		ID3D11Buffer* hwCamDataBuf = &camDataBuf->GetHardwareBuffer();
		deviceContext.PSSetConstantBuffers(1, 1, &hwCamDataBuf);
		
		for (const RenderItemDrawCall& drawCall : mPerFrameDrawCalls)
		{
			// try to draw something
			mDevice->GetDeviceContext().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			DX11GFXConstantBuffer* modelMatBuf = mDevice->GetConstantBuffer(drawCall.ConstantBuffer);
			ID3D11Buffer* hwModelMatBuf = &modelMatBuf->GetHardwareBuffer();
			deviceContext.VSSetConstantBuffers(1, 1, &hwModelMatBuf);

			DX11GFXConstantBuffer* materialBuf = mDevice->GetConstantBuffer(drawCall.MaterialDataBuffer);
			ID3D11Buffer* hwMaterialBuf = &materialBuf->GetHardwareBuffer();
			deviceContext.PSSetConstantBuffers(2, 1, &hwMaterialBuf);

			ID3D11Buffer* vertBuf = &mDevice->GetVertexBuffer(drawCall.VertexBuffer)->GetHardwareBuffer();
			
			struct TempDataLayoutStructure
			{
				Vector3D position;
 				Vector3D normal;
 				Vector2D uv;
 				Vector3D tangents;
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
		ImGui::Render();
		mDevice->Present();
	}

	void Renderer::ShutDown()
	{
		ImGui_ImplDX11_Shutdown();
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

		DX11GFXConstantBuffer* lightBuf = mDevice->GetConstantBuffer(mLightBuf);
		AssertExpr(!mLightingList.empty());
		lightBuf->UpdateSubresources(&mLightingList[0]);

		DX11GFXConstantBuffer* camDataBuf = mDevice->GetConstantBuffer(mCameraDataBuf);
		camDataBuf->UpdateSubresources(&camera.Position);

		mPerFrameDrawCalls.clear();
		void* tmpMatrixBuf = malloc(sizeof(Matrix4x4) * 2);
		for (RenderItemProtocol& renderItem : mRenderItems)
		{
			// #TODO(Josh::This needs to be removed -- an opaque handle should be leased out that will
			//             get fixed up when we remove render items)
			if (!renderItem.bIsValid)
			{
				continue;
			}

			memcpy(tmpMatrixBuf, renderItem.ModelMatrix.GetValuePtr(), sizeof(Matrix4x4));
			memcpy((U8*)tmpMatrixBuf + sizeof(Matrix4x4), renderItem.ModelMatrix.Inverse().GetValuePtr(), sizeof(Matrix4x4));
			DX11GFXConstantBuffer* constBuf = mDevice->GetConstantBuffer(renderItem.ConstantBuffer);
			constBuf->UpdateSubresources(tmpMatrixBuf);

			for (size_t index = 0; index < renderItem.MeshData.size(); ++index)
			{
				RenderItemMeshData& meshData = renderItem.MeshData[index];

				RenderItemDrawCall drawCall;
				drawCall.VertexBuffer = meshData.VertexBuffer;
				drawCall.IndexBuffer = meshData.IndexBuffer;
				// #TODO(Deal with this later. This is the same for all meshes of renderItem)
				drawCall.ConstantBuffer = renderItem.ConstantBuffer;

 				drawCall.TextureSlot0 = meshData.TextureDescs[0].TextureBuffer; // This should be iterated on to be more robust.
 				drawCall.TextureSlot1 = meshData.TextureDescs[1].TextureBuffer; // This should be iterated on to be more robust.
 				drawCall.TextureSlot2 = meshData.TextureDescs[2].TextureBuffer; // This should be iterated on to be more robust.

				// #HACK(This should be a 1:1... Just until I completely rework the data layout for render items.)
				drawCall.MaterialDataBuffer = renderItem.MaterialBuffers[index];

				DX11GFXConstantBuffer* materialBuf = mDevice->GetConstantBuffer(drawCall.MaterialDataBuffer);
				materialBuf->UpdateSubresources(&meshData.Material);

				mPerFrameDrawCalls.push_back(std::move(drawCall));
			}
		}
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

		ImGui::GetIO().DisplaySize.x = static_cast<float>(width);
		ImGui::GetIO().DisplaySize.y = static_cast<float>(height);

		//mPassGraph->OnWindowResize(width, height);

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
