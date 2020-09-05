#pragma once

#include <bitset>
#include <vector>

#include <Utils/PrimitiveDefs.h>
#include <Utils/DebugUtils/Debug.h>

// 64 bits of component glory
#define ENTITY_MAX_COMPONENTS 64

namespace Apollo
{
	typedef U32 EntityID;

	const EntityID kInvalidEntityID = 0;

	typedef std::bitset<ENTITY_MAX_COMPONENTS> ComponentBitset;
	struct Entity
	{
		EntityID mEntityID;
		ComponentBitset mComponentSet;
	};
}
