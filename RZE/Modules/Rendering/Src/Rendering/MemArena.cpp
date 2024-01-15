#include <Rendering/MemArena.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/PrimitiveDefs.h>

namespace Rendering
{
	namespace MemArena
	{
		Byte* s_producerBuf = nullptr;
		Byte* s_consumerBuf = nullptr;

		struct ArenaState
		{
			size_t curPos = 0;
			size_t size = 0;
		};
		ArenaState s_arenaState;

		void ValidateArenaState(size_t sizeRequested)
		{
			// validate state. can't alloc if these rules arent met.
			if (s_arenaState.curPos + sizeRequested > s_arenaState.size)
			{
				size_t sizeAvailable = s_arenaState.size - s_arenaState.curPos;
				RZE_LOG_ARGS("Rendering memory arena has run out of space. Size requested: %zu available: %zu ", sizeRequested, sizeAvailable);
				AssertExpr(s_arenaState.curPos + sizeRequested < s_arenaState.size);
			}
		}

		void InitializeArena(size_t size)
		{
			if (s_producerBuf != nullptr || s_consumerBuf != nullptr)
			{
				RZE_LOG("Rendering memory arena being re-initialized. \
					This is incorrect as the memory should be valid once for the length of a Rendering::Renderer lifetime.");
				AssertIsNull(s_producerBuf);
				AssertIsNull(s_consumerBuf);
			}
			
			s_producerBuf = new Byte[size];
			s_consumerBuf = new Byte[size];

			memset(s_producerBuf, 0, size);
			memset(s_consumerBuf, 0, size);

			s_arenaState.size = size;
		}

		void Shutdown()
		{
			if (s_producerBuf == nullptr)
			{
				AssertMsg(s_consumerBuf == nullptr, "If one is null, they must both be null or something got weird.");
				delete s_producerBuf;
			}

			if (s_consumerBuf == nullptr)
			{
				AssertMsg(s_producerBuf == nullptr, "If one is null, they must both be null or something got weird.");
				delete s_consumerBuf;
			}
		}

		void* Alloc(size_t size)
		{
			ValidateArenaState(size);

			Byte* allocMem = &s_producerBuf[s_arenaState.curPos];
			memset(allocMem, 0, size);
			s_arenaState.curPos += size;

			return allocMem;
		}

		Byte* GetConsumerPtr()
		{
			return s_consumerBuf;
		}

		void Cycle()
		{
			s_arenaState.curPos = 0;

			std::swap(s_producerBuf, s_consumerBuf);
		}
	}
}