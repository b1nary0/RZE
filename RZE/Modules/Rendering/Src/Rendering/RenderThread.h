#pragma once

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include <Rendering/GPUProfiler.h>
#include <Rendering/RenderCommand.h>

#include <Utils/PrimitiveDefs.h>
#include <Utils/StatsWindow.h>

namespace Rendering
{
	class IGFXDevice;
	class IImGuiRenderer;
	struct GFXDeviceOptions;
	enum class EGPUStatisticsMode;

	// The render thread's frames, averaged over a StatsWindow, for display, and the last frame alone, for measuring
	// (FrameTimeCapture). A frame runs from one present to the next; the frame time itself is the main thread's
	// (FrameStats).
	struct RenderThreadStats
	{
		// Per frame, running commands, Present included; the rest of the frame is spent waiting for the main thread
		double BusyTimeMS = 0.0;
		// The part of BusyTimeMS spent in Present, which can block on the GPU or the display
		double PresentTimeMS = 0.0;
		// The same for the last frame the render thread presented
		double LastFrameBusyTimeMS = 0.0;
		double LastFramePresentTimeMS = 0.0;

		// The GPU's time for a frame, from its first command to its last before presenting (GPUProfiler), averaged over
		// the frames read back in the window, and for the newest frame read back, which is a few frames old
		double GPUFrameTimeMS = 0.0;
		double LastFrameGPUTimeMS = 0.0;
		// Each GPU scope's time a frame (Renderer::BeginGPUScope), by id, averaged the same way
		std::vector<double> GPUScopeTimesMS;

		// Whether the GPU's pipeline statistics can be counted: not with EGPUStatisticsMode::Off, nor on a device that
		// has none (known once they're first turned on)
		bool CanCountGPUStatistics = false;
		// Whether the window's averages below come from counted frames (Renderer::SetGPUStatistics, or
		// EGPUStatisticsMode::Always). Without any, they're zero.
		bool HasGPUStatistics = false;
		// What the GPU counted a frame in each scope, by id, averaged over the window's counted frames; in the frame's
		// scopes altogether (GPUFrameProfile::FrameStatistics), the same way; and in the newest frame read back's scopes,
		// zero if it wasn't counted
		std::vector<GPUPipelineStatistics> GPUScopeStatistics;
		GPUPipelineStatistics GPUFrameStatistics;
		GPUPipelineStatistics LastFrameGPUStatistics;
	};

	class RenderThread
	{
	public:
		RenderThread();

	public:
		// The thread that calls Initialize controls this one: Shutdown and RunWhileIdle must be called from it
		void Initialize(void* windowHandle, const GFXDeviceOptions& deviceOptions);
		void Shutdown();

		void PushCommand(RenderCommand* command);

		// Hands the commands pushed since the last call to the render thread. Waits for it to finish the previous batch,
		// then cycles MemArena, swaps the producer and consumer queues and wakes it. Called once a frame (Renderer).
		void SignalProcess();

		// Blocks until the swap chain can queue another frame, or timeoutMS passes (false). Safe from any thread.
		bool WaitForPresentSlot(U32 timeoutMS);

		// Zero until the first window of frames has passed. Safe from any thread.
		RenderThreadStats GetStats() const;

		// Blocks until the render thread has finished processing its queue, then has the render thread run func, and
		// returns once it has. For the rare work that needs the device between frames: it runs where the device is
		// attached, with nothing else using it. Only from the controlling thread.
		void RunWhileIdle(std::function<void()> func);

		// The device, for making resources (its Create functions), which is safe from the main thread
		IGFXDevice& GetDevice();
		// ImGui's renderer, for RunWhileIdle work and NewFrame
		IImGuiRenderer& GetImGuiRenderer();

	private:
		// One pass of the render thread: a batch of commands or a RunWhileIdle task. False once Shutdown has signalled.
		bool Update();

		void InitializeImGui();

		void ProcessCommands();
		// Adds the batch of commands that started at batchStart to the current frame, which ends if the batch presented
		void UpdateStats(double batchStart, bool hasPresented);
		// A frame the GPU timer read back, into the window and the last frame's GPU time
		void AddGPUFrame(const GPUFrameProfile& profile);

		// What each command does (RenderThreadCommands.cpp). ProcessCommands calls the one for its type.
		// The frame
		void Execute(RenderCommand_ImGuiRender& command);
		void Execute(RenderCommand_BeginFrame& command);
		void Execute(RenderCommand_EndFrame& command);
		void Execute(RenderCommand_Begin& command);
		void Execute(RenderCommand_End& command);
		void Execute(RenderCommand_DevicePresent& command);
		void Execute(RenderCommand_BeginGPUScope& command);
		void Execute(RenderCommand_EndGPUScope& command);
		void Execute(RenderCommand_HandleWindowResize& command);
		void Execute(RenderCommand_SetSyncInterval& command);
		void Execute(RenderCommand_SetGPUStatistics& command);

		// Resources
		void Execute(RenderCommand_CreateVertexBuffer& command);
		void Execute(RenderCommand_CreateIndexBuffer& command);
		void Execute(RenderCommand_CreateConstantBuffer& command);
		void Execute(RenderCommand_CreateTextureBuffer2D& command);
		void Execute(RenderCommand_CreateVertexShader& command);
		void Execute(RenderCommand_CreatePixelShader& command);
		void Execute(RenderCommand_CreateComputeShader& command);
		void Execute(RenderCommand_UploadDataToBuffer& command);
		void Execute(RenderCommand_UpdateVertexBuffer& command);
		void Execute(RenderCommand_ReleaseVertexShader& command);
		void Execute(RenderCommand_ReleasePixelShader& command);
		void Execute(RenderCommand_ReleaseComputeShader& command);

		// Targets
		void Execute(RenderCommand_ClearDepthStencilBuffer& command);
		void Execute(RenderCommand_ClearRenderTargets& command);
		void Execute(RenderCommand_SetRenderTargetBackBuffer& command);
		void Execute(RenderCommand_SetDepthTarget& command);
		void Execute(RenderCommand_SetColourTarget& command);
		void Execute(RenderCommand_GenerateMips& command);
		void Execute(RenderCommand_CopyTexture& command);
		void Execute(RenderCommand_CopyTextureRegion& command);
		void Execute(RenderCommand_CopyTextureSlice& command);
		void Execute(RenderCommand_ClearRenderTarget& command);

		// Pipeline state
		void Execute(RenderCommand_SetViewport& command);
		void Execute(RenderCommand_SetInputLayout& command);
		void Execute(RenderCommand_SetPrimitiveTopology& command);
		void Execute(RenderCommand_SetRasterizerState& command);
		void Execute(RenderCommand_SetBlendState& command);
		void Execute(RenderCommand_SetDepthState& command);
		void Execute(RenderCommand_SetVertexShader& command);
		void Execute(RenderCommand_SetPixelShader& command);
		void Execute(RenderCommand_UnsetPixelShader& command);
		void Execute(RenderCommand_SetComputeShader& command);
		void Execute(RenderCommand_SetConstantBufferVS& command);
		void Execute(RenderCommand_SetConstantBufferPS& command);
		void Execute(RenderCommand_SetConstantBufferCS& command);
		void Execute(RenderCommand_SetVertexBuffer& command);
		void Execute(RenderCommand_SetIndexBuffer& command);
		void Execute(RenderCommand_SetTextureResource& command);
		void Execute(RenderCommand_UnsetTextureResource& command);
		void Execute(RenderCommand_SetTextureResourceCS& command);
		void Execute(RenderCommand_UnsetTextureResourceCS& command);
		void Execute(RenderCommand_SetUnorderedAccess& command);
		void Execute(RenderCommand_UnsetUnorderedAccess& command);

		// Draws
		void Execute(RenderCommand_Draw& command);
		void Execute(RenderCommand_DrawIndexed& command);
		void Execute(RenderCommand_DrawIndexedInstanced& command);
		void Execute(RenderCommand_DrawFullScreenQuad& command);

		// Compute
		void Execute(RenderCommand_Dispatch& command);

		// Reading back
		void Execute(RenderCommand_ReadTexel& command);
		void Execute(RenderCommand_ReadTexture& command);
		void Execute(RenderCommand_ReadBackBuffer& command);

		// Named regions for PIX and similar tools; no-ops when no profiler is attached
		void BeginProfilerEvent(const char* name);
		void EndProfilerEvent();

	private:
		// The commands themselves live in MemArena, which keeps a batch's memory until the render thread has run it.
		// Commands run in the order they were pushed. Vectors rather than std::queue so they keep their
		// capacity between frames: the consumer is cleared, not freed, after its commands run.
		std::vector<RenderCommand*> m_producerQueue;
		std::vector<RenderCommand*> m_consumerQueue;

		void* m_windowHandle = nullptr;
		std::unique_ptr<IGFXDevice> m_device;
		std::unique_ptr<IImGuiRenderer> m_imguiRenderer;

		std::thread m_thread;
		// The thread that called Initialize. Update relies on Shutdown and RunWhileIdle both running on it, so an idle
		// task is never pending when the shutdown signal arrives.
		std::thread::id m_controllingThreadID;
		std::mutex m_updateMutex;

		std::condition_variable m_updateCondition;
		bool m_processSignal = false;
		bool m_shuttingDown = false;
		// RunWhileIdle's work, run by the render thread instead of a batch; empty when there's none
		std::function<void()> m_idleTask;

		// Whether a profiler (e.g. PIX) is listening for markers; refreshed for each batch of commands
		bool m_emitProfilerMarkers = false;

		// Whether the bound vertex and compute shaders were created. A shader that failed to compile is empty
		// (DX11ShaderTypes, GLShaderTypes), and draws and dispatches without one are skipped: drivers can hang on them.
		bool m_vertexShaderBound = false;
		bool m_computeShaderBound = false;

		// The frame so far, on the render thread, since the last present
		double m_frameBusyTime = 0.0;
		double m_framePresentTime = 0.0;
		// The stats window so far, on the render thread. It starts at the first present.
		StatsWindow m_statsWindow;
		double m_windowBusyTime = 0.0;
		double m_windowPresentTime = 0.0;
		// The GPU frames read back in the stats window so far, their total time and their scopes' totals by id
		U32 m_windowGPUFrames = 0;
		double m_windowGPUTime = 0.0;
		std::vector<double> m_windowGPUScopeTimes;
		// The statistics the same way, over the frames that counted them
		U32 m_windowGPUStatisticsFrames = 0;
		std::vector<GPUPipelineStatistics> m_windowGPUScopeStatistics;
		GPUPipelineStatistics m_windowGPUFrameStatistics;
		double m_lastGPUFrameTime = 0.0;
		GPUPipelineStatistics m_lastGPUFrameStatistics;

		GPUProfiler m_gpuProfiler;
		// GFXDeviceOptions::GPUStatistics. Set before the render thread starts, then read only by it.
		EGPUStatisticsMode m_gpuStatisticsMode{};
		// What m_gpuProfiler reads back at each present, kept so its vectors keep their memory
		GPUFrameProfile m_gpuFrameProfile;

		// Written by the render thread at every present (the last frame's) and once per window (the averages), read from
		// any thread
		RenderThreadStats m_stats;
		mutable std::mutex m_statsMutex;
	};
}