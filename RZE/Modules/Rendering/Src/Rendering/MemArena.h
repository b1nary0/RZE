#pragma once

#include <Utils/PrimitiveDefs.h>

namespace Rendering
{
	namespace MemArena
	{
		void InitializeArena(size_t size);

		template <typename TAlloc>
		TAlloc* AllocType()
		{
			TAlloc* memory = new (Alloc(sizeof(TAlloc))) TAlloc();
			return memory;
		}

		void* Alloc(size_t sizeRequested);

		Byte* GetConsumerPtr();

		// Cycle will reset the arena for fresh use.
		void Cycle();
	}
}