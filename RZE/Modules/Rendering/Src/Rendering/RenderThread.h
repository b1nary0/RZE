#pragma once

#include <queue>

namespace Rendering
{
	struct RenderCommand;

	// @note not threaded atm, but storing the functionality here, then we
	// will  basically signal the thread to process in its update function
	class RenderThread
	{
	public:
		RenderThread();

	public:
		void Update();

		void PushCommand(RenderCommand* command);
		void ProcessCommands();

		// @todo note sure about the future of this, but just getting an idea down
		void SignalProcess();

	protected:
	private:
		// @note these are guaranteed to be contiguous as its backed by MemArena
		std::queue<RenderCommand*> m_commandQueue;
	};
}