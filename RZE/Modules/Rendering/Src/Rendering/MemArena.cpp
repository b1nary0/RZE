#include <Rendering/MemArena.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/PrimitiveDefs.h>

#include <cstring>
#include <memory>
#include <utility>
#include <vector>

#define PRESSURE_LIMIT_MED 60
#define PRESSURE_LIMIT_HIGH 90

namespace Rendering
{
	namespace MemArena
	{
		// One of the two buffers Cycle() swaps: the arena's memory, and the allocations that didn't fit in it while it
		// was the producer. Those come from the heap and are held until the buffer is the producer again, when the
		// render thread has finished with its commands.
		struct ArenaBuffer
		{
			std::unique_ptr<Byte[]> memory;
			std::vector<std::unique_ptr<Byte[]>> overflow;
		};

		static ArenaBuffer s_producer;
		static ArenaBuffer s_consumer;

		static struct ArenaState
		{
			size_t curPos = 0;
			size_t size = 0;
			// The total amount of memory allocated this frame, overflow included
			size_t frameTotalMemAllocCurrFrame = 0;
			size_t frameTotalMemAllocLastFrame = 0;
			// What went to the heap this frame because the arena was full
			size_t overflowBytesCurrFrame = 0;
			size_t overflowAllocsCurrFrame = 0;
			// The highest frame total encountered during a runtime session
			size_t peakUsedBytes = 0;
			PressureValue pressureValue = PressureValue::LOW;
		} s_arenaState;

		static PressureValue CalculatePressureValue()
		{
			// Of the frame Cycle() just ended
			const float pressurePercentage = (static_cast<float>(s_arenaState.frameTotalMemAllocLastFrame) / s_arenaState.size) * 100;

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

			if (s_producer.memory != nullptr || s_consumer.memory != nullptr)
			{
				RZE_LOG_ERROR("Rendering memory arena being re-initialized. \
					This is incorrect as the memory should be valid once for the length of a Rendering::Renderer lifetime.");
				AssertIsNull(s_producer.memory);
				AssertIsNull(s_consumer.memory);
			}

			// make_unique<T[]> value-initializes, so both buffers start zeroed.
			s_producer.memory = std::make_unique<Byte[]>(size);
			s_consumer.memory = std::make_unique<Byte[]>(size);

			s_arenaState.size = size;
		}

		void Shutdown()
		{
			AssertMsg(s_producer.memory != nullptr && s_consumer.memory != nullptr, "MemArena::Shutdown called without InitializeArena");

			// Back to the state before InitializeArena, so it can run again
			s_producer = ArenaBuffer();
			s_consumer = ArenaBuffer();
			s_arenaState = ArenaState();
		}

		void* Alloc(size_t size)
		{
			s_arenaState.frameTotalMemAllocCurrFrame += size;
			if (s_arenaState.peakUsedBytes < s_arenaState.frameTotalMemAllocCurrFrame)
			{
				s_arenaState.peakUsedBytes = s_arenaState.frameTotalMemAllocCurrFrame;
			}

			// Written as a subtraction so a huge size can't wrap the sum; curPos never passes size
			if (size > s_arenaState.size - s_arenaState.curPos)
			{
				// Full: the allocation comes from the heap, zeroed like the arena's, until this buffer is reused
				s_arenaState.overflowBytesCurrFrame += size;
				++s_arenaState.overflowAllocsCurrFrame;
				s_producer.overflow.push_back(std::make_unique<Byte[]>(size));
				return s_producer.overflow.back().get();
			}

			Byte* allocMem = &s_producer.memory[s_arenaState.curPos];
			memset(allocMem, 0, size);
			s_arenaState.curPos += size;

			return allocMem;
		}

		void Cycle()
		{
			if (s_arenaState.overflowAllocsCurrFrame > 0)
			{
				RZE_LOG_WARNING_ARGS("Rendering memory arena was full: %zu KB in %zu allocations went to the heap this frame (arena size %zu KB)",
					s_arenaState.overflowBytesCurrFrame / 1024, s_arenaState.overflowAllocsCurrFrame, s_arenaState.size / 1024);
			}

			s_arenaState.curPos = 0;
			s_arenaState.frameTotalMemAllocLastFrame = s_arenaState.frameTotalMemAllocCurrFrame;
			s_arenaState.frameTotalMemAllocCurrFrame = 0;
			s_arenaState.overflowBytesCurrFrame = 0;
			s_arenaState.overflowAllocsCurrFrame = 0;

			s_arenaState.pressureValue = CalculatePressureValue();

			std::swap(s_producer, s_consumer);

			// The new producer's commands are the batch the render thread just finished (SignalProcess waits for it
			// before cycling), so nothing reads what spilled from it any more
			s_producer.overflow.clear();
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