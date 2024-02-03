#pragma once

#include <Utils/PrimitiveDefs.h>

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

		// Cycle will reset the arena for fresh use.
		void Cycle();

		Byte* GetConsumerPtr();
		size_t GetTotalMemoryAllocatedLastFrame();
		size_t GetSize();
		// Returns the highest total bytes used during a runtime session
		size_t GetPeakUsedBytes();
		PressureValue GetPressureValue();

		void* Alloc(size_t sizeRequested);

		template <typename TAlloc>
		TAlloc* AllocType()
		{
			TAlloc* memory = new (Alloc(sizeof(TAlloc))) TAlloc();
			return memory;
		}
	}
}