#include <RZE_Config.h>
#if !WITH_NEW_RENDERER
#include <Rendering/LegacyRenderer.h>

#include <Optick/optick.h>

#include <Rendering/Graphics/GFXPassGraph.h>

#include <Perseus/JobSystem/JobScheduler.h>

#include <Utils/Conversions.h>
#include <Utils/MemoryUtils.h>
#include <Utils/DebugUtils/Debug.h>
#include <Utils/Math/Vector4D.h>
#include <Utils/Platform/FilePath.h>

// DX11
#include <Rendering/Driver/DX11/DX11GFXDevice.h>
#include <Rendering/Driver/DX11/DX11.h>
// #TODO(What makes this needed here? unique_ptr complains re: destructor visibility
//       is it necessary otherwise?)
#include <Rendering/Driver/DX11/DX11GFXVertexBuffer.h>
#include <Rendering/Driver/DX11/DX11GFXIndexBuffer.h>
#include <Rendering/Driver/DX11/DX11GFXConstantBuffer.h>
#include <Rendering/Driver/DX11/DX11GFXTextureBuffer2D.h>
#include <Rendering/Graphics/RenderTarget.h>


#include <ImGui/imgui.h>
#include <ImGui/imgui_impl_dx11.h>
#include <ImGui/imgui_impl_win32.h>

#include <array>

namespace Rendering
{
	static void* gCPUMatrixBuf = nullptr;

	LegacyRenderer::LegacyRenderer()
		: mPassGraph(std::make_unique<GFXPassGraph>())
		, mRenderTarget(nullptr)
	{
		mLightingList.reserve(MAX_LIGHTS);

		// Creating two renderers, stomping static data.
		AssertNull(gCPUMatrixBuf);
		gCPUMatrixBuf = malloc(sizeof(Matrix4x4) * 2);
	}

	LegacyRenderer::~LegacyRenderer()
	{
		// Something bad went wrong here.
		AssertNotNull(gCPUMatrixBuf);
		free(gCPUMatrixBuf);
	}

	Int32 LegacyRenderer::AddRenderItem(const RenderItemProtocol& itemProtocol)
	{
		U32 constantBufferID = mDevice->CreateConstantBuffer(sizeof(Matrix4x4), 2);

		std::vector<U32> meshMaterialBuffers;
		meshMaterialBuffers.reserve(itemProtocol.MeshData.size());
		for (size_t index = 0; index < itemProtocol.MeshData.size(); ++index)
		{
			meshMaterialBuffers.push_back(mDevice->CreateConstantBuffer(MemoryUtils::AlignSize(sizeof(RenderItemMaterialDesc), 16), 1));
		}

		if (!mFreeRenderListIndices.empty())
		{
			Int32 index = mFreeRenderListIndices.front();
			mFreeRenderListIndices.pop();

			RenderItemProtocol& renderProtocol = mRenderItems[index];
			renderProtocol = std::move(itemProtocol);
			renderProtocol.bIsValid = true;
			renderProtocol.ConstantBuffer = constantBufferID;
			renderProtocol.MaterialBuffers = std::move(meshMaterialBuffers);

			return index;
		}

		mRenderItems.emplace_back(std::move(itemProtocol));
		RenderItemProtocol& renderProtocol = mRenderItems.back();
		renderProtocol.bIsValid = true;
		renderProtocol.ConstantBuffer = constantBufferID;
		renderProtocol.MaterialBuffers = std::move(meshMaterialBuffers);

		return static_cast<Int32>(mRenderItems.size() - 1);
	}

	void LegacyRenderer::RemoveRenderItem(const U32 itemIdx)
	{
		AssertExpr(itemIdx < mRenderItems.size());

		mRenderItems[itemIdx].Invalidate();
		mFreeRenderListIndices.push(itemIdx);

		// #TODO(Return constant buffer for render item here)
	}

	Int32 LegacyRenderer::AddLightItem(const LightItemProtocol& itemProtocol)
	{
		mLightingList.emplace_back(std::move(itemProtocol));

		++mLightCounts[itemProtocol.LightType];
		return static_cast<Int32>(mLightingList.size() - 1);
	}

	void LegacyRenderer::RemoveLightItem(const U32 itemIdx)
	{
		AssertExpr(itemIdx < mLightingList.size());

		auto iter = mLightingList.begin() + itemIdx;
		--mLightCounts[iter->LightType];
		mLightingList.erase(iter);
	}

	// #TODO
	// Take with new Renderer
	void LegacyRenderer::Initialize()
	{
		mCanvasSize.SetXY(1600, 900);

		mDevice = std::make_unique<DX11GFXDevice>();
		mDevice->SetWindow(mWindowHandle);
		mDevice->Initialize();

		ImGui::CreateContext();
		ImGui_ImplWin32_Init(mWindowHandle);
		ImGui_ImplDX11_Init(&mDevice->GetHardwareDevice(), &mDevice->GetDeviceContext());

		mPassGraph->Build(this);
	}

	void LegacyRenderer::Update()
	{
		OPTICK_EVENT();

		ProcessCommands();
		PrepareDrawCalls();

		{
			// #TODO(Eventually this should be moved out of here and into the engine level.
			//       Then the render side just deals with the generated commands.)
			mPassGraph->Execute();
		}
	}

	void LegacyRenderer::Render()
	{
		if (mRenderTarget != nullptr)
		{
			ID3D11RenderTargetView* nullViews[] = { nullptr };
			mDevice->GetDeviceContext().OMSetRenderTargets(ARRAYSIZE(nullViews), nullViews, nullptr);

			mDevice->GetDeviceContext().OMSetRenderTargets(1, &mDevice->mRenderTargetView, mDevice->mDepthStencilView);

			FLOAT rgba[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
			mDevice->GetDeviceContext().ClearRenderTargetView(mDevice->mRenderTargetView, rgba);
		}

		{
			OPTICK_EVENT("ImGui::Render");
			ImGui::Render();
			ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		}

		mDevice->Present();
	}

	void LegacyRenderer::ShutDown()
	{
		ImGui_ImplDX11_Shutdown();
		mDevice->Shutdown();
	}

	const std::vector<LegacyRenderer::RenderItemDrawCall>& LegacyRenderer::GetDrawCalls()
	{
		return mPerFrameDrawCalls;
	}

	const std::vector<LegacyRenderer::LightItemProtocol>& LegacyRenderer::GetLights()
	{
		return mLightingList;
	}

	const U32* LegacyRenderer::GetLightCounts()
	{
		return mLightCounts;
	}

	const Rendering::LegacyRenderer::CameraItemProtocol& LegacyRenderer::GetCamera()
	{
		return camera;
	}

	void LegacyRenderer::PrepareDrawCalls()
	{
		OPTICK_EVENT();

		if (mPerFrameDrawCalls.capacity() < mRenderItems.size())
		{
			mPerFrameDrawCalls.reserve(mRenderItems.size());
		}

		mPerFrameDrawCalls.clear();
		
		for (RenderItemProtocol& renderItem : mRenderItems)
		{
			// #TODO(Josh::This needs to be removed -- an opaque handle should be leased out that will
			//             get fixed up when we remove render items)
			if (!renderItem.bIsValid)
			{
				continue;
			}

			for (size_t index = 0; index < renderItem.MeshData.size(); ++index)
			{
				RenderItemMeshData& meshData = renderItem.MeshData[index];

				RenderItemDrawCall drawCall;
				drawCall.VertexBuffer = meshData.VertexBuffer;
				drawCall.IndexBuffer = meshData.IndexBuffer;
				// #TODO(Deal with this later. This is the same for all meshes of renderItem)
				drawCall.ConstantBuffer = renderItem.ConstantBuffer;

				drawCall.IsTextured = meshData.Material.IsTextured;
				if (meshData.Material.IsTextured)
				{
					drawCall.TextureSlot0 = meshData.TextureDescs[0].TextureBuffer; // This should be iterated on to be more robust.
					drawCall.TextureSlot1 = meshData.TextureDescs[1].TextureBuffer; // This should be iterated on to be more robust.
					drawCall.TextureSlot2 = meshData.TextureDescs[2].TextureBuffer; // This should be iterated on to be more robust.
				}

				// #HACK(This should be a 1:1... Just until I completely rework the data layout for render items.)
				drawCall.MaterialDataBuffer = renderItem.MaterialBuffers[index];

				DX11GFXConstantBuffer* materialBuf = mDevice->GetConstantBuffer(drawCall.MaterialDataBuffer);
				materialBuf->UpdateSubresources(&meshData.Material);

				mPerFrameDrawCalls.emplace_back(std::move(drawCall));
			}
		}
	}

	// #TODO
	// Take with new renderer - only used in hacky RenderTarget code in EditorApp
	Rendering::DX11GFXDevice& LegacyRenderer::GetDriverDevice()
	{
		AssertNotNull(mDevice);
		return *mDevice;
	}

	// #TODO
	// Take with new Renderer
	void LegacyRenderer::EnableVsync(bool bEnabled)
	{
		mDevice->SetSyncInterval(static_cast<U32>(bEnabled));
	}
	
	const Vector2D& LegacyRenderer::GetCanvasSize()
	{
		return mCanvasSize;
	}

	// #TODO
	// Take with new renderer
	void LegacyRenderer::ResizeCanvas(const Vector2D& newSize)
	{
		mCanvasSize = newSize;

		ImGui::GetIO().DisplaySize.x = static_cast<float>(mCanvasSize.X());
		ImGui::GetIO().DisplaySize.y = static_cast<float>(mCanvasSize.Y());

		mDevice->HandleWindowResize(newSize);
		mPassGraph->OnWindowResize(static_cast<int>(mCanvasSize.X()), static_cast<int>(mCanvasSize.Y()));

		LOG_CONSOLE_ARGS("New Canvas Size: %f x %f", mCanvasSize.X(), mCanvasSize.Y());
	}

	// #TODO
	// Take with new renderer - only used in hacky RenderTarget code in EditorApp
	void LegacyRenderer::SetViewportSize(const Vector2D& newSize)
	{
		mViewportDimensions = newSize;
	}

	const Vector2D& LegacyRenderer::GetViewportSize()
	{
		return mViewportDimensions;
	}

	// #TODO
	// Take with new renderer - only used in hacky RenderTarget code in EditorApp
	void LegacyRenderer::SetRenderTarget(RenderTargetTexture* renderTarget)
	{
		AssertNotNull(renderTarget);
		mRenderTarget = renderTarget;
	}

	Rendering::RenderTargetTexture* LegacyRenderer::GetRenderTarget()
	{
		return mRenderTarget;
	}

	U32 LegacyRenderer::CreateVertexBuffer(void* data, size_t size, U32 count)
	{
		return mDevice->CreateVertexBuffer(data, size, count);
	}

	U32 LegacyRenderer::CreateIndexBuffer(void* data, size_t size, U32 count)
	{
		return mDevice->CreateIndexBuffer(data, size, count);
	}

	U32 LegacyRenderer::CreateTextureBuffer2D(void* data, U32 width, U32 height)
	{
		// This is temp here, should be passed in or something.
		GFXTextureBufferParams params = { 0 };
		params.bIsRenderTarget = true;
		params.bIsShaderResource = true;
		params.Height = height;
		params.Width = width;
		params.MipLevels = 0;
		params.MostDetailedMip = 0;
		params.SampleCount = 1;
		params.SampleQuality = 0;
		return mDevice->CreateTextureBuffer2D(data, params);
	}

	// #TODO
	// Take with new renderer (neutered)
	U32 LegacyRenderer::QueueCreateVertexBufferCommand(void* data, size_t size, U32 count)
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

	U32 LegacyRenderer::QueueCreateIndexBufferCommand(void* data, size_t size, U32 count)
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

	U32 LegacyRenderer::QueueCreateTextureCommand(ECreateTextureBufferType bufferType, void* data, U32 width, U32 height)
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


	void LegacyRenderer::QueueUpdateRenderItem(U32 itemID, const Matrix4x4& worldMtx)
	{
		std::lock_guard<std::mutex> lock(mUpdateRenderItemWorldMatrixCommandMutex);

		UpdateRenderItemWorldMatrixCommand command;
		command.RenderItemID = itemID;
		command.WorldMtx = worldMtx;

		mUpdateRenderItemWorldMatrixCommandQueue.push_back(std::move(command));
	}

	void LegacyRenderer::ProcessCommands()
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

				// Doing this check here for now because its the easiest place to have access to the old and new matrices.
				// Should architect it so this can be done before we even calculate anything.
				// #TODO(This caused the item to not show up when it's brand new because the matrices will not be different.. fix.)
				//if (command.WorldMtx != renderItem.ModelMatrix)
				{
					renderItem.ModelMatrix = std::move(command.WorldMtx);

					memcpy(gCPUMatrixBuf, renderItem.ModelMatrix.GetValuePtr(), sizeof(Matrix4x4));
					memcpy((U8*)gCPUMatrixBuf + sizeof(Matrix4x4), renderItem.ModelMatrix.Inverse().GetValuePtr(), sizeof(Matrix4x4));
					DX11GFXConstantBuffer* constBuf = mDevice->GetConstantBuffer(renderItem.ConstantBuffer);
					constBuf->UpdateSubresources(gCPUMatrixBuf);
				}
			}
			mUpdateRenderItemWorldMatrixCommandQueue.clear();
			
		}
	}

	void LegacyRenderer::RenderItemProtocol::Invalidate()
	{
		bIsValid = false;
	}

}
#endif
