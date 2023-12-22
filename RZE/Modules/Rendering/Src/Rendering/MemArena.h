#pragma once

namespace Rendering
{
	namespace MemArena
	{
		void InitializeArena(size_t size);

		template <typename TAlloc>
		TAlloc* AllocType()
		{
			void* memory = Alloc(sizeof(TAlloc));
			memset(memory, 0, sizeof(TAlloc));

			return reinterpret_cast<TAlloc*>(memory);
		}

		void* Alloc(size_t sizeRequested);
	}
}