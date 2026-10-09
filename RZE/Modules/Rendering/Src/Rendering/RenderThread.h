#pragma once

#include <mutex>
#include <thread>
#include <queue>

namespace Rendering
{
	struct RenderCommand;

	class DX11Device;

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

	private:
		// @note these are guaranteed to be contiguous as its backed by MemArena
		std::queue<RenderCommand*> m_producerQueue;
		std::queue<RenderCommand*> m_consumerQueue;

		void* m_windowHandle = nullptr;
		std::unique_ptr<DX11Device> m_device = nullptr;

		std::thread m_thread;
		std::mutex m_updateMutex;

		std::condition_variable m_updateCondition;
		bool m_processSignal = false;
		bool m_shuttingDown = false;
	};
}