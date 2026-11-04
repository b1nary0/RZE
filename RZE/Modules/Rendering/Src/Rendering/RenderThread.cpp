#include <StdAfx.h>
#include <Rendering/RenderThread.h>

#include <Rendering/MemArena.h>

#include <Rendering/Driver/GFXDevice.h>
#include <Rendering/Driver/GFXDeviceFactory.h>
#include <Rendering/ImGui/ImGuiRenderer.h>

#include <Utils/TimeUtils.h>
#include <Utils/DebugUtils/Debug.h>

#include <Optick/optick.h>

#include <imgui/imgui.h>

namespace Rendering
{
	RenderThread::RenderThread()
	{
	}

	void RenderThread::Initialize(void* windowHandle)
	{
		AssertNotNull(windowHandle);

		m_windowHandle = windowHandle;

		m_device = GFXDeviceFactory::CreateGFXDevice();
		m_device->SetWindow(m_windowHandle);
		m_device->Initialize();
		m_imguiRenderer = GFXDeviceFactory::CreateImGuiRenderer(*m_device);

		InitializeImGui();

		// The render thread uses the device from here on
		m_device->DetachFromCurrentThread();
		auto threadExec = [this]()
			{
				OPTICK_THREAD("Render Thread");
				m_device->AttachToCurrentThread();
				while (!m_shuttingDown)
				{
					Update();
				}
				m_device->DetachFromCurrentThread();
			};

		m_thread = std::thread(threadExec);
	}

	void RenderThread::Shutdown()
	{
		{
			// @todo need to wrap this it's ugly and see if we can API-ifize our usage of this pattern in general
			// this block is to ensure the proper shutdown thread synchronization
			std::unique_lock waitlock(m_updateMutex);
			m_updateCondition.wait(waitlock, [this]() { return m_processSignal == false; });
			m_processSignal = true;
			waitlock.unlock();
			m_updateCondition.notify_all();
		}

		m_shuttingDown = true;
		m_thread.join();

		// Back on the main thread for the rest
		m_device->AttachToCurrentThread();
		m_imguiRenderer->Shutdown();
		// Saves imgui.ini, through io.IniFilename and any settings handlers the application added, which point into the
		// application. It's never deleted (RZE_Engine::m_application), so they're still there after its ShutDown.
		ImGui::DestroyContext();
		m_imguiRenderer.reset();
		m_device->Shutdown();
	}

	bool RenderThread::WaitForPresentSlot(U32 timeoutMS)
	{
		return m_device->WaitForPresentSlot(timeoutMS);
	}

	void RenderThread::RunWhileIdle(std::function<void()> func)
	{
		std::unique_lock waitlock(m_updateMutex);
		m_updateCondition.wait(waitlock, [this]() { return m_processSignal == false && !m_idleTask; });
		m_idleTask = std::move(func);
		m_updateCondition.notify_all();
		m_updateCondition.wait(waitlock, [this]() { return !m_idleTask; });
	}

	IGFXDevice& RenderThread::GetDevice()
	{
		AssertNotNull(m_device);
		return *m_device;
	}

	IImGuiRenderer& RenderThread::GetImGuiRenderer()
	{
		AssertNotNull(m_imguiRenderer);
		return *m_imguiRenderer;
	}

	RenderThreadStats RenderThread::GetStats() const
	{
		std::lock_guard lock(m_statsMutex);
		return m_stats;
	}

	void RenderThread::InitializeImGui()
	{
		ImGui::CreateContext();
		m_imguiRenderer->Initialize();
	}

	void RenderThread::Update()
	{
		std::unique_lock lock(m_updateMutex);
		{
			OPTICK_EVENT("Wait");
			m_updateCondition.wait(lock, [this] { return m_processSignal || m_idleTask; });
		}

		// RunWhileIdle's caller waits while this runs, holding nothing else
		if (m_idleTask)
		{
			m_idleTask();
			m_idleTask = nullptr;
			lock.unlock();
			m_updateCondition.notify_all();
			return;
		}

		if (!m_shuttingDown)
		{
			ProcessCommands();
		}

		m_processSignal = false;
		lock.unlock();
		m_updateCondition.notify_all();
	}

	void RenderThread::PushCommand(RenderCommand* command)
	{
		AssertNotNull(command);
		m_producerQueue.push_back(command);
	}

	void RenderThread::ProcessCommands()
	{
		OPTICK_EVENT();
		AssertExpr(m_processSignal == true);

		const double batchStart = TimeUtils::GetSeconds();
		bool hasPresented = false;

		// Markers are only made when a profiler is listening (some APIs convert each name).
		// Checked once per batch: a batch is a whole frame, so every BeginEvent below still gets its EndEvent.
		m_emitProfilerMarkers = m_device->IsProfilerAttached();

		for (RenderCommand* command : m_consumerQueue)
		{
			switch (command->type)
			{
			case RenderCommandType::ImGuiRender: Execute(static_cast<RenderCommand_ImGuiRender&>(*command)); break;
			case RenderCommandType::BeginFrame: Execute(static_cast<RenderCommand_BeginFrame&>(*command)); break;
			case RenderCommandType::EndFrame: Execute(static_cast<RenderCommand_EndFrame&>(*command)); break;
			case RenderCommandType::Begin: Execute(static_cast<RenderCommand_Begin&>(*command)); break;
			case RenderCommandType::End: Execute(static_cast<RenderCommand_End&>(*command)); break;
			case RenderCommandType::DevicePresent: Execute(static_cast<RenderCommand_DevicePresent&>(*command)); hasPresented = true; break;
			case RenderCommandType::HandleWindowResize: Execute(static_cast<RenderCommand_HandleWindowResize&>(*command)); break;
			case RenderCommandType::SetSyncInterval: Execute(static_cast<RenderCommand_SetSyncInterval&>(*command)); break;
			case RenderCommandType::CreateVertexBuffer: Execute(static_cast<RenderCommand_CreateVertexBuffer&>(*command)); break;
			case RenderCommandType::CreateIndexBuffer: Execute(static_cast<RenderCommand_CreateIndexBuffer&>(*command)); break;
			case RenderCommandType::CreateConstantBuffer: Execute(static_cast<RenderCommand_CreateConstantBuffer&>(*command)); break;
			case RenderCommandType::CreateTextureBuffer2D: Execute(static_cast<RenderCommand_CreateTextureBuffer2D&>(*command)); break;
			case RenderCommandType::CreateVertexShader: Execute(static_cast<RenderCommand_CreateVertexShader&>(*command)); break;
			case RenderCommandType::CreatePixelShader: Execute(static_cast<RenderCommand_CreatePixelShader&>(*command)); break;
			case RenderCommandType::CreateComputeShader: Execute(static_cast<RenderCommand_CreateComputeShader&>(*command)); break;
			case RenderCommandType::UploadDataToBuffer: Execute(static_cast<RenderCommand_UploadDataToBuffer&>(*command)); break;
			case RenderCommandType::UpdateVertexBuffer: Execute(static_cast<RenderCommand_UpdateVertexBuffer&>(*command)); break;
			case RenderCommandType::ReleaseVertexShader: Execute(static_cast<RenderCommand_ReleaseVertexShader&>(*command)); break;
			case RenderCommandType::ReleasePixelShader: Execute(static_cast<RenderCommand_ReleasePixelShader&>(*command)); break;
			case RenderCommandType::ReleaseComputeShader: Execute(static_cast<RenderCommand_ReleaseComputeShader&>(*command)); break;
			case RenderCommandType::ClearDepthStencilBuffer: Execute(static_cast<RenderCommand_ClearDepthStencilBuffer&>(*command)); break;
			case RenderCommandType::ClearRenderTargets: Execute(static_cast<RenderCommand_ClearRenderTargets&>(*command)); break;
			case RenderCommandType::SetRenderTargetBackBuffer: Execute(static_cast<RenderCommand_SetRenderTargetBackBuffer&>(*command)); break;
			case RenderCommandType::SetDepthTarget: Execute(static_cast<RenderCommand_SetDepthTarget&>(*command)); break;
			case RenderCommandType::SetColourTarget: Execute(static_cast<RenderCommand_SetColourTarget&>(*command)); break;
			case RenderCommandType::GenerateMips: Execute(static_cast<RenderCommand_GenerateMips&>(*command)); break;
			case RenderCommandType::CopyTexture: Execute(static_cast<RenderCommand_CopyTexture&>(*command)); break;
			case RenderCommandType::CopyTextureRegion: Execute(static_cast<RenderCommand_CopyTextureRegion&>(*command)); break;
			case RenderCommandType::ClearRenderTarget: Execute(static_cast<RenderCommand_ClearRenderTarget&>(*command)); break;
			case RenderCommandType::SetViewport: Execute(static_cast<RenderCommand_SetViewport&>(*command)); break;
			case RenderCommandType::SetInputLayout: Execute(static_cast<RenderCommand_SetInputLayout&>(*command)); break;
			case RenderCommandType::SetPrimitiveTopology: Execute(static_cast<RenderCommand_SetPrimitiveTopology&>(*command)); break;
			case RenderCommandType::SetRasterizerState: Execute(static_cast<RenderCommand_SetRasterizerState&>(*command)); break;
			case RenderCommandType::SetBlendState: Execute(static_cast<RenderCommand_SetBlendState&>(*command)); break;
			case RenderCommandType::SetDepthState: Execute(static_cast<RenderCommand_SetDepthState&>(*command)); break;
			case RenderCommandType::SetVertexShader: Execute(static_cast<RenderCommand_SetVertexShader&>(*command)); break;
			case RenderCommandType::SetPixelShader: Execute(static_cast<RenderCommand_SetPixelShader&>(*command)); break;
			case RenderCommandType::UnsetPixelShader: Execute(static_cast<RenderCommand_UnsetPixelShader&>(*command)); break;
			case RenderCommandType::SetComputeShader: Execute(static_cast<RenderCommand_SetComputeShader&>(*command)); break;
			case RenderCommandType::SetConstantBufferVS: Execute(static_cast<RenderCommand_SetConstantBufferVS&>(*command)); break;
			case RenderCommandType::SetConstantBufferPS: Execute(static_cast<RenderCommand_SetConstantBufferPS&>(*command)); break;
			case RenderCommandType::SetConstantBufferCS: Execute(static_cast<RenderCommand_SetConstantBufferCS&>(*command)); break;
			case RenderCommandType::SetVertexBuffer: Execute(static_cast<RenderCommand_SetVertexBuffer&>(*command)); break;
			case RenderCommandType::SetIndexBuffer: Execute(static_cast<RenderCommand_SetIndexBuffer&>(*command)); break;
			case RenderCommandType::SetTextureResource: Execute(static_cast<RenderCommand_SetTextureResource&>(*command)); break;
			case RenderCommandType::UnsetTextureResource: Execute(static_cast<RenderCommand_UnsetTextureResource&>(*command)); break;
			case RenderCommandType::SetTextureResourceCS: Execute(static_cast<RenderCommand_SetTextureResourceCS&>(*command)); break;
			case RenderCommandType::UnsetTextureResourceCS: Execute(static_cast<RenderCommand_UnsetTextureResourceCS&>(*command)); break;
			case RenderCommandType::SetUnorderedAccess: Execute(static_cast<RenderCommand_SetUnorderedAccess&>(*command)); break;
			case RenderCommandType::UnsetUnorderedAccess: Execute(static_cast<RenderCommand_UnsetUnorderedAccess&>(*command)); break;
			case RenderCommandType::Draw: Execute(static_cast<RenderCommand_Draw&>(*command)); break;
			case RenderCommandType::DrawIndexed: Execute(static_cast<RenderCommand_DrawIndexed&>(*command)); break;
			case RenderCommandType::DrawFullScreenQuad: Execute(static_cast<RenderCommand_DrawFullScreenQuad&>(*command)); break;
			case RenderCommandType::Dispatch: Execute(static_cast<RenderCommand_Dispatch&>(*command)); break;
			case RenderCommandType::ReadTexel: Execute(static_cast<RenderCommand_ReadTexel&>(*command)); break;
			case RenderCommandType::ReadTexture: Execute(static_cast<RenderCommand_ReadTexture&>(*command)); break;
			case RenderCommandType::ReadBackBuffer: Execute(static_cast<RenderCommand_ReadBackBuffer&>(*command)); break;

			default:
			{
				RZE_LOG_ERROR_ARGS("[Rendering::Renderer] Unknown RenderCommandType encountered: %d", static_cast<int>(command->type));
				AssertFalse();
			}
			}

			command->~RenderCommand();
		}

		// Keeps the capacity, so steady-state frames don't allocate
		m_consumerQueue.clear();

		UpdateStats(batchStart, hasPresented);
	}

	void RenderThread::UpdateStats(double batchStart, bool hasPresented)
	{
		const double now = TimeUtils::GetSeconds();
		m_frameBusyTime += now - batchStart;
		if (!hasPresented)
		{
			return;
		}

		// The first present only starts the first window
		if (m_statsWindow.HasStarted())
		{
			m_windowBusyTime += m_frameBusyTime;
			m_windowPresentTime += m_framePresentTime;
		}
		m_frameBusyTime = 0.0;
		m_framePresentTime = 0.0;

		if (!m_statsWindow.AddFrame(now))
		{
			return;
		}

		{
			std::lock_guard lock(m_statsMutex);
			m_stats.BusyTimeMS = m_statsWindow.PerFrame(m_windowBusyTime) * 1000.0;
			m_stats.PresentTimeMS = m_statsWindow.PerFrame(m_windowPresentTime) * 1000.0;
		}

		m_windowBusyTime = 0.0;
		m_windowPresentTime = 0.0;
	}

	void RenderThread::BeginProfilerEvent(const char* name)
	{
		if (m_emitProfilerMarkers)
		{
			m_device->BeginProfilerEvent(name);
		}
	}

	void RenderThread::EndProfilerEvent()
	{
		if (m_emitProfilerMarkers)
		{
			m_device->EndProfilerEvent();
		}
	}

	void RenderThread::SignalProcess()
	{
		{
			OPTICK_EVENT("Wait | RenderThread::SignalProcess");
			std::unique_lock waitlock(m_updateMutex);
			m_updateCondition.wait(waitlock, [this]() { return m_processSignal == false; });
		}

		AssertExpr(m_processSignal == false);

		MemArena::Cycle();
		std::swap(m_producerQueue, m_consumerQueue);

		std::lock_guard<std::mutex> lock(m_updateMutex);
		m_processSignal = true;
		m_updateCondition.notify_all();
	}
}