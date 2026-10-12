#pragma once

#include <Utils/PrimitiveDefs.h>

#include <string>

// A GameObject's persistent identity: random when the object is created, then saved and loaded with it, so it's the
// same every session. Unlike GameObjectID, which is only unique within a session, it's what scene files and anything
// else that outlives a session refer to objects by. 0 is never generated.
using ObjectGUID = U64;

namespace ObjectGUIDs
{
	constexpr ObjectGUID k_invalid = 0;

	// A new random GUID. Safe on any thread.
	ObjectGUID Generate();

	// As 16 hex digits, the form scene files store; JSON numbers can't hold every 64-bit value exactly
	std::string ToString(ObjectGUID guid);
	// k_invalid if text isn't a GUID written by ToString
	ObjectGUID FromString(const char* text);
}
