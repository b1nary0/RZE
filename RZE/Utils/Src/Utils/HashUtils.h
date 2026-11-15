#pragma once

#include <Utils/PrimitiveDefs.h>

#include <initializer_list>

namespace HashUtils
{
	// FNV-1a over the bits of the values: the same values always hash the same, and values that differ in any bit hash
	// differently (-0.0f from 0.0f), barring a collision, which is as unlikely as 64 bits make it. The order counts.
	U64 HashFloats(std::initializer_list<float> values);
}
