#include <Rendering/MemArena.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/PrimitiveDefs.h>

#define PRESSURE_LIMIT_MED 60
#define PRESSURE_LIMIT_HIGH 90

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
			// The total amount of memory allocated this frame
			size_t frameTotalMemAllocCurrFrame = 0;
			size_t frameTotalMemAllocLastFrame = 0;
			// The highest size encountered during a runtime session
			size_t peakUsedBytes = 0;
			PressureValue pressureValue = PressureValue::LOW;
		};
		static ArenaState s_arenaState;

		static void ValidateArenaState(size_t sizeRequested)
		{
			// validate state. can't alloc if these rules arent met.
			if (s_arenaState.curPos + sizeRequested > s_arenaState.size)
			{
				size_t sizeAvailable = s_arenaState.size - s_arenaState.curPos;
				RZE_LOG_ARGS("Rendering memory arena has run out of space. Size requested: %zu available: %zu ", sizeRequested, sizeAvailable);
				AssertExpr(s_arenaState.curPos + sizeRequested < s_arenaState.size);
			}
		}
		
		static PressureValue CalculatePressureValue()
		{
			const float pressurePercentage = (static_cast<float>(s_arenaState.frameTotalMemAllocCurrFrame) / s_arenaState.size) * 100;

			if (pressurePercentage >= PRESSURE_LIMIT_HIGH)
			{
				return PressureValue::HIGH;
			}
			else if (pressurePercentage >= PRESSURE_LIMIT_MED)
			{
				return PressureValue::MED;
			}
			else
			{
				return PressureValue::LOW;
			}
		}

		void InitializeArena(size_t size)
		{
			AssertExpr(size > 0);

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
			AssertMsg(s_consumerBuf != nullptr && s_producerBuf != nullptr, "If one is valid, they must both be valid or something got weird.");

			delete s_producerBuf;
			s_producerBuf = nullptr;

			delete s_consumerBuf;
			s_consumerBuf = nullptr;
		}

		void* Alloc(size_t size)
		{
			ValidateArenaState(size);

			Byte* allocMem = &s_producerBuf[s_arenaState.curPos];
			memset(allocMem, 0, size);

			s_arenaState.curPos += size;
			s_arenaState.frameTotalMemAllocCurrFrame += size;

			if (s_arenaState.peakUsedBytes < s_arenaState.curPos)
			{
				s_arenaState.peakUsedBytes = s_arenaState.curPos;
			}

			return allocMem;
		}

		Byte* GetConsumerPtr()
		{
			return s_consumerBuf;
		}

		void Cycle()
		{
			s_arenaState.curPos = 0;
			s_arenaState.frameTotalMemAllocLastFrame = s_arenaState.frameTotalMemAllocCurrFrame;
			s_arenaState.frameTotalMemAllocCurrFrame = 0;

			s_arenaState.pressureValue = CalculatePressureValue();

			std::swap(s_producerBuf, s_consumerBuf);
		}

		size_t GetTotalMemoryAllocatedLastFrame()
		{
			return s_arenaState.frameTotalMemAllocLastFrame;
		}

		size_t GetSize()
		{
			return s_arenaState.size;
		}

		size_t GetPeakUsedBytes()
		{
			return s_arenaState.peakUsedBytes;
		}

		PressureValue GetPressureValue()
		{
			return s_arenaState.pressureValue;
		}
	}
}