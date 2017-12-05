#pragma once

#include <bitset>
#include <vector>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/PrimitiveDefs.h>

// 64 bits of component glory
#define ENTITY_MAX_COMPONENTS 64

namespace Apollo
{
	typedef U32 EntityID;

	typedef std::bitset<ENTITY_MAX_COMPONENTS> ComponentBitset;
	struct Entity
	{
		U32 mEntityID;
		ComponentBitset mComponentSet;
	};
}
