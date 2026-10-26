#pragma once

#include <mutex>
#include <thread>
#include <vector>

#include <Utils/PrimitiveDefs.h>
#include <Utils/StatsWindow.h>

namespace Rendering
{
	struct RenderCommand;

	class DX11Device;

	// The render thread's frames, averaged over a StatsWindow, for display. A frame runs from one present to the next;
	// the frame time itself is the main thread's (FrameStats).
	struct RenderThreadStats
	{
		// Per frame, running commands, Present included; the rest of the frame is spent waiting for the main thread
		double BusyTimeMS = 0.0;
		// The part of BusyTimeMS spent in Present, which can block on the GPU or the display
		double PresentTimeMS = 0.0;
	};

	class RenderThread
	{
	public:
		RenderThread();

	public:
		void Initialize(void* windowHandle);
		void Shutdown();

		void PushCommand(RenderCommand* command);

		// @todo note sure about the future of this, but just getting an idea down
		void SignalProcess();

		// Blocks until the swap chain can queue another frame, or timeoutMS passes (false). Safe from any thread.
		bool WaitForPresentSlot(U32 timeoutMS);

		// Zero until the first window of frames has passed. Safe from any thread.
		RenderThreadStats GetStats() const;

		// Blocks until the render thread has finished processing its queue, then runs func on the calling
		// thread while the render thread is held idle. Use for the rare case where the calling thread
		// needs exclusive access to the device context.
		template <typename Func>
		void RunWhileIdle(Func&& func)
		{
			std::unique_lock waitlock(m_updateMutex);
			m_updateCondition.wait(waitlock, [this]() { return m_processSignal == false; });
			func();
		}

	private:
		void Update();

		void InitializeImGui();

		void ProcessCommands();
		// Adds the batch of commands that started at batchStart to the current frame, which ends if the batch presented
		void UpdateStats(double batchStart, bool hasPresented);

		// Named regions for PIX and similar tools; no-ops when no profiler is attached
		void BeginProfilerEvent(const char* name);
		void EndProfilerEvent();

	private:
		// @note these are guaranteed to be contiguous as its backed by MemArena
		// Commands run in the order they were pushed. Vectors rather than std::queue so they keep their
		// capacity between frames: the consumer is cleared, not freed, after its commands run.
		std::vector<RenderCommand*> m_producerQueue;
		std::vector<RenderCommand*> m_consumerQueue;

		void* m_windowHandle = nullptr;
		std::unique_ptr<DX11Device> m_device = nullptr;
		// The swap chain's frame latency HANDLE (DX11Device::GetFrameLatencyWaitableObject); fixed while the thread runs
		void* m_frameLatencyWaitableObject = nullptr;

		std::thread m_thread;
		std::mutex m_updateMutex;

		std::condition_variable m_updateCondition;
		bool m_processSignal = false;
		bool m_shuttingDown = false;

		// Whether a profiler (e.g. PIX) is listening for D3DPERF markers; refreshed for each batch of commands
		bool m_emitProfilerMarkers = false;

		// The frame so far, on the render thread, since the last present
		double m_frameBusyTime = 0.0;
		double m_framePresentTime = 0.0;
		// The stats window so far, on the render thread. It starts at the first present.
		StatsWindow m_statsWindow;
		double m_windowBusyTime = 0.0;
		double m_windowPresentTime = 0.0;

		// Written by the render thread once per window, read from any thread
		RenderThreadStats m_stats;
		mutable std::mutex m_statsMutex;
	};
}