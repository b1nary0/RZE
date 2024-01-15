#pragma once

#include <queue>

namespace Rendering
{
	struct RenderCommand;

	class DX11Device;

	// @note not threaded atm, but storing the functionality here, then we
	// will  basically signal the thread to process in its update function
	class RenderThread
	{
	public:
		RenderThread();

	public:
		void Initialize(void* windowHandle);
		void Shutdown();

		void Update();

		void PushCommand(RenderCommand* command);
		void ProcessCommands();

		// @todo note sure about the future of this, but just getting an idea down
		void SignalProcess();

	private:
		void InitializeImGui();

	private:
		// @note these are guaranteed to be contiguous as its backed by MemArena
		std::queue<RenderCommand*> m_producerQueue;
		std::queue<RenderCommand*> m_consumerQueue;

		void* m_windowHandle = nullptr;
		std::unique_ptr<DX11Device> m_device = nullptr;
	};
}