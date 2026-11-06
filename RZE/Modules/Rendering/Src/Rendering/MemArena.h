#pragma once

#include <Utils/PrimitiveDefs.h>

#include <cstddef>

namespace Rendering
{
	namespace MemArena
	{
		enum class PressureValue
		{
			LOW,
			MED,
			HIGH
		};

		void InitializeArena(size_t size);
		void Shutdown();

		// Cycle will reset the arena for fresh use, and frees what overflowed from the buffer it hands out next.
		void Cycle();

		// Overflow included, so it can be more than GetSize()
		size_t GetTotalMemoryAllocatedLastFrame();
		size_t GetSize();
		// Returns the highest frame total during a runtime session, overflow included
		size_t GetPeakUsedBytes();
		// Of the last frame: its total against the arena's size
		PressureValue GetPressureValue();

		// Zeroed memory that lasts until the render thread has run this frame's commands. When the arena is full, it
		// comes from the heap instead, and Cycle() logs a warning for the frame.
		void* Alloc(size_t sizeRequested);

		template <typename TAlloc>
		TAlloc* AllocType()
		{
			TAlloc* memory = new (Alloc(sizeof(TAlloc))) TAlloc();
			return memory;
		}
	}
}