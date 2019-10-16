#include <Diotima/Renderer.h>

#include <Optick/optick.h>

#include <Diotima/Graphics/GFXPassGraph.h>

#include <Perseus/JobSystem/JobScheduler.h>

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

#include <ImGui/imgui.h>
#include <ImGui/imgui_impl_dx12.h>

namespace Diotima
{
	Renderer::Renderer()
		: mPassGraph(std::make_unique<GFXPassGraph>())
		, mMatrixConstantBuffer(nullptr)
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

			return index;
		}

		mRenderItems.emplace_back(std::move(itemProtocol));
		mRenderItems.back().bIsValid = true;

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

		mMatrixConstantBuffer = malloc(sizeof(Matrix4x4) * 3);

		DX12Initialize();

		mPassGraph->Build(this);
	}

	void Renderer::Update()
	{
		OPTICK_EVENT();

		mDevice->ResetCommandAllocator();
		mDevice->ResetResourceCommandAllocator();

		ProcessCommands();

		PrepareDrawCalls();

		{
			// #TODO(Eventually this should be moved out of here and into the engine level.
			//       Then the render side just deals with the generated commands.)
			mPassGraph->Execute();
		}

		HRESULT res = mDevice->GetDevice()->GetDeviceRemovedReason();
		if (res != S_OK)
		{
			AssertFalse();
		}
	}

	void Renderer::Render()
	{
		mDevice->Present();
	}

	void Renderer::ShutDown()
	{
		free(mMatrixConstantBuffer);

		ImGui_ImplDX12_Shutdown();

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

	void Renderer::DX12Initialize()
	{
		mDevice = std::make_unique<DX12GFXDevice>();
		mDevice->SetWindow(mWindowHandle);
		mDevice->SetMSAASampleCount(mMSAASampleCount);
		mDevice->Initialize();

		mMVPConstantBuffer = mDevice->CreateConstantBuffer(sizeof(Matrix4x4) * 3, 65536);
		mMaterialBuffer = mDevice->CreateConstantBuffer(sizeof(RenderItemMaterialDesc), 65536);
	}

	void Renderer::PrepareDrawCalls()
	{
		OPTICK_EVENT();

		if (mPerFrameDrawCalls.capacity() < mRenderItems.size())
		{
			mPerFrameDrawCalls.reserve(mRenderItems.size());
		}

		Matrix4x4 camViewProjMat = camera.ProjectionMat * camera.ViewMat;

		DX12GFXConstantBuffer* const materialBuffer = mDevice->GetConstantBuffer(mMaterialBuffer);
		DX12GFXConstantBuffer* const matrixBuffer = mDevice->GetConstantBuffer(mMVPConstantBuffer);

		matrixBuffer->Reset();
		materialBuffer->Reset();

		mPerFrameDrawCalls.clear();
		for (RenderItemProtocol& renderItem : mRenderItems)
		{
			// #TODO(Josh::This needs to be removed -- an opaque handle should be leased out that will
			//             get fixed up when we remove render items)
			if (!renderItem.bIsValid)
			{
				continue;
			}

			CBAllocationData matrixSlot;
			// #TODO(Josh::Just copy pasta from before for now to get RenderItemDrawCall driving the Renderer::Update loop
				//             Should find a better place for it.)
			{
				// Create and copy a buffer of matrices for a given item
				// Camera matrix should not be here but will be moved 
				// elsewhere when the frame design stabilizes
				const float* modelViewPtr = renderItem.ModelMatrix.GetValuePtr();
				const float* modelViewInvPtr = renderItem.ModelMatrix.Inverse().GetValuePtr();
				const float* camViewProjPtr = camViewProjMat.GetValuePtr();

				memcpy(mMatrixConstantBuffer, modelViewPtr, sizeof(Matrix4x4));
				memcpy((U8*)mMatrixConstantBuffer + sizeof(Matrix4x4), modelViewInvPtr, sizeof(Matrix4x4));
				memcpy((U8*)mMatrixConstantBuffer + sizeof(Matrix4x4) * 2, camViewProjPtr, sizeof(Matrix4x4));

				matrixSlot = matrixBuffer->AllocateMember(mMatrixConstantBuffer);
			}

			for (RenderItemMeshData& meshData : renderItem.MeshData)
			{
				RenderItemDrawCall drawCall;
				drawCall.VertexBuffer = meshData.VertexBuffer;
				drawCall.IndexBuffer = meshData.IndexBuffer;
				// #TODO(Josh::Eventually the material system will handle this itself and hold a buffer of pre-allocated materials
				//             and we will somehow "lease" it for the draw call)
				drawCall.MaterialSlot = materialBuffer->AllocateMember(&meshData.Material);

				drawCall.TextureSlot0 = meshData.TextureDescs[0].TextureBuffer; // This should be iterated on to be more robust.
				drawCall.TextureSlot1 = meshData.TextureDescs[1].TextureBuffer; // This should be iterated on to be more robust.
				drawCall.TextureSlot2 = meshData.TextureDescs[2].TextureBuffer; // This should be iterated on to be more robust.

				drawCall.MatrixSlot = matrixSlot;

				mPerFrameDrawCalls.push_back(std::move(drawCall));
			}
		}

		memset(mMatrixConstantBuffer, 0, sizeof(Matrix4x4) * 3);
	}

	void Renderer::EnableVsync(bool bEnabled)
	{
		mDevice->SetSyncInterval(static_cast<U32>(bEnabled));
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

		mDevice->HandleWindowResize(width, height);

		mPassGraph->OnWindowResize(width, height);

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

	U32 Renderer::QueueCreateVertexBufferCommand(void* data, U32 numElements)
	{
		std::lock_guard<std::mutex> lock(mVertexBufferCommandMutex);

		CreateBufferRenderCommand command;
		command.BufferType = ECreateBufferType::Vertex;
		command.Data = data;
		command.NumElements = numElements;
		mVertexBufferCommandQueue.push_back(std::move(command));

		// #TODO(This is shit, fix later)
		return mDevice->GetVertexBufferCount() + mVertexBufferCommandQueue.size() - 1;
	}

	U32 Renderer::QueueCreateIndexBufferCommand(void* data, U32 numElements)
	{
		std::lock_guard<std::mutex> lock(mIndexBufferCommandMutex);

		CreateBufferRenderCommand command;
		command.BufferType = ECreateBufferType::Index;
		command.Data = data;
		command.NumElements = numElements;
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
				CreateVertexBuffer(command.Data, command.NumElements);
			}
			mVertexBufferCommandQueue.clear();
		}

		{
			OPTICK_EVENT("Process CreateIndexBuffer commands");
			std::lock_guard<std::mutex> lock(mIndexBufferCommandMutex);
			for (auto& command : mIndexBufferCommandQueue)
			{
				CreateIndexBuffer(command.Data, command.NumElements);
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
