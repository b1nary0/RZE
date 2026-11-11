#include <Rendering/MemArena.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/Memory/MemoryUtils.h>
#include <Utils/PrimitiveDefs.h>

#include <cstddef>
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

			// Left uninitialized, not make_unique's zeroes: Alloc zeroes each allocation, so nothing reads memory it hasn't
			// zeroed, and the OS only backs the pages the frames actually use (zeroing all of it made the whole arena
			// resident).
			s_producer.memory = std::unique_ptr<Byte[]>(new Byte[size]);
			s_consumer.memory = std::unique_ptr<Byte[]>(new Byte[size]);

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

		static void AddToFrameTotal(size_t bytes)
		{
			s_arenaState.frameTotalMemAllocCurrFrame += bytes;
			if (s_arenaState.peakUsedBytes < s_arenaState.frameTotalMemAllocCurrFrame)
			{
				s_arenaState.peakUsedBytes = s_arenaState.frameTotalMemAllocCurrFrame;
			}
		}

		void* Alloc(size_t size, size_t alignment)
		{
			// Both buffers come from new[], which aligns for max_align_t, so an aligned offset is an aligned address
			AssertMsg(alignment <= alignof(std::max_align_t), "MemArena can't align past max_align_t");
			const size_t alignedPos = MemoryUtils::AlignSize(s_arenaState.curPos, alignment);

			// Written without adding to size, so a huge size can't wrap the sum
			const bool fits = alignedPos <= s_arenaState.size && size <= s_arenaState.size - alignedPos;
			if (!fits)
			{
				// Full: the allocation comes from the heap, zeroed and aligned like the arena's, until this buffer is
				// reused. It needs no padding.
				AddToFrameTotal(size);
				s_arenaState.overflowBytesCurrFrame += size;
				++s_arenaState.overflowAllocsCurrFrame;
				s_producer.overflow.push_back(std::make_unique<Byte[]>(size));
				return s_producer.overflow.back().get();
			}

			// The padding counts as used, so without a spill the frame's total is the arena's position
			AddToFrameTotal(alignedPos - s_arenaState.curPos + size);

			Byte* allocMem = s_producer.memory.get() + alignedPos;
			memset(allocMem, 0, size);
			s_arenaState.curPos = alignedPos + size;

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