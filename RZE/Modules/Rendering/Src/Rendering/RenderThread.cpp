#include <StdAfx.h>
#include <Rendering/RenderThread.h>

#include <Rendering/MemArena.h>
#include <Rendering/RenderCommand.h>

#include <Utils/DebugUtils/Debug.h>

#include <Optick/optick.h>

#define MEM_ARENA_SIZE 1024

namespace Rendering
{
	static bool k_processSignal = false;

	static Byte* s_processingMemory = nullptr;

	RenderThread::RenderThread()
	{
		// @todo move this to an Initialize function
		// as to better control init flow. RAII is trash, sorry
		s_processingMemory = new Byte[1024];

		MemArena::InitializeArena(MEM_ARENA_SIZE);
	}

	void RenderThread::Update()
	{
		if (k_processSignal)
		{
			{
				OPTICK_EVENT("RenderCommand memcpy");
				memcpy(s_processingMemory, MemArena::Get(), MEM_ARENA_SIZE);
			}

			ProcessCommands();

			k_processSignal = false;
		}
	}

	void RenderThread::PushCommand(RenderCommand* command)
	{
		AssertNotNull(command);
		m_commandQueue.push(command);
	}

	void RenderThread::ProcessCommands()
	{
		AssertExpr(k_processSignal = true);
		k_processSignal = false;

		RenderCommand* currCommand = reinterpret_cast<RenderCommand*>(s_processingMemory);
		while (currCommand)
		{
			switch (currCommand->type)
			{
			case RenderCommandType::CreateVertexBuffer:
				break;

			case RenderCommandType::CreateIndexBuffer:
				break;

			case RenderCommandType::CreateConstantBuffer:
				break;

			case RenderCommandType::CreateTextureBuffer2D:
				break;

			default:
				RZE_LOG_ARGS("[Rendering::Renderer] Unknown RenderCommandType encountered: %d", currCommand->type);
				AssertFalse();
			}
		}
	}
	void RenderThread::SignalProcess()
	{
		AssertExpr(k_processSignal = false);
		k_processSignal = true;
	}
}