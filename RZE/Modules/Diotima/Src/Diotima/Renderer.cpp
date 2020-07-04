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
#include <Diotima/Graphics/RenderTarget.h>


#include <ImGui/imgui.h>
#include <ImGui/imgui_impl_dx11.h>
#include <ImGui/imgui_impl_win32.h>

#include <array>

namespace Diotima
{

	Renderer::Renderer()
		: mPassGraph(std::make_unique<GFXPassGraph>())
		, mRenderTarget(nullptr)
	{
	}

	Renderer::~Renderer()
	{
	}

	void Renderer::Initialize()
	{
		mCanvasSize.SetXY(1600, 900);

		mDevice = std::make_unique<DX11GFXDevice>();
		mDevice->SetWindow(mWindowHandle);
		mDevice->Initialize();

		ImGui::CreateContext();
		ImGui_ImplWin32_Init(mWindowHandle);
		ImGui_ImplDX11_Init(&mDevice->GetHardwareDevice(), &mDevice->GetDeviceContext());

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
			mPassGraph->Execute();
		}
	}

	void Renderer::Render()
	{
		if (mRenderTarget != nullptr)
		{
			ID3D11RenderTargetView* nullViews[] = { nullptr };
			mDevice->GetDeviceContext().OMSetRenderTargets(ARRAYSIZE(nullViews), nullViews, nullptr);

			mDevice->GetDeviceContext().OMSetRenderTargets(1, &mDevice->mRenderTargetView, mDevice->mDepthStencilView);

			FLOAT rgba[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
			mDevice->GetDeviceContext().ClearRenderTargetView(mDevice->mRenderTargetView, rgba);
		}

		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		mDevice->Present();
	}

	void Renderer::ShutDown()
	{
		ImGui_ImplDX11_Shutdown();
		mDevice->Shutdown();
	}

	Diotima::DX11GFXDevice& Renderer::GetDriverDevice()
	{
		AssertNotNull(mDevice);
		return *mDevice;
	}

	void Renderer::EnableVsync(bool bEnabled)
	{
		mDevice->SetSyncInterval(static_cast<U32>(bEnabled));
	}
	
	const Vector2D& Renderer::GetCanvasSize()
	{
		return mCanvasSize;
	}

	// #TODO
	// Take with new renderer
	void Renderer::ResizeCanvas(const Vector2D& newSize)
	{
		mCanvasSize = newSize;

		int width = static_cast<int>(newSize.X());
		int height = static_cast<int>(newSize.Y());

		ImGui::GetIO().DisplaySize.x = static_cast<float>(width);
		ImGui::GetIO().DisplaySize.y = static_cast<float>(height);

		mDevice->HandleWindowResize(newSize);
		mPassGraph->OnWindowResize(width, height);

		LOG_CONSOLE_ARGS("New Canvas Size: %f x %f", mCanvasSize.X(), mCanvasSize.Y());
	}

	// #TODO
	// Take with new renderer - only used in hacky RenderTarget code in EditorApp
	void Renderer::SetViewportSize(const Vector2D& newSize)
	{
		mViewportDimensions = newSize;
	}

	const Vector2D& Renderer::GetViewportSize()
	{
		return mViewportDimensions;
	}

	// #TODO
	// Take with new renderer - only used in hacky RenderTarget code in EditorApp
	void Renderer::SetRenderTarget(RenderTargetTexture* renderTarget)
	{
		AssertNotNull(renderTarget);
		mRenderTarget = renderTarget;
	}

	Diotima::RenderTargetTexture* Renderer::GetRenderTarget()
	{
		return mRenderTarget;
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
	U32 Renderer::QueueCreateVertexBufferCommand(void* data, size_t size, U32 count)
	{
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
			for (auto& command : mVertexBufferCommandQueue)
			{
				CreateVertexBuffer(command.Data, command.Size, command.Count);
			}
			mVertexBufferCommandQueue.clear();
		}

		{
			OPTICK_EVENT("Process CreateIndexBuffer commands");
			for (auto& command : mIndexBufferCommandQueue)
			{
				CreateIndexBuffer(command.Data, command.Size, command.Count);
			}
			mIndexBufferCommandQueue.clear();
		}

		{
			OPTICK_EVENT("Process CreateTextureBuffer commands");
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
			void* tmpMatrixBuf = malloc(sizeof(Matrix4x4) * 2);
			for (auto& command : mUpdateRenderItemWorldMatrixCommandQueue)
			{
				
			}
			mUpdateRenderItemWorldMatrixCommandQueue.clear();
			free(tmpMatrixBuf);
		}
	}

}
