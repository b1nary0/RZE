#include <Utils/HashUtils.h>

#include <cstring>

namespace
{
	constexpr U64 k_fnvOffsetBasis = 14695981039346656037ull;
	constexpr U64 k_fnvPrime = 1099511628211ull;
}

namespace HashUtils
{
	U64 HashFloats(std::initializer_list<float> values)
	{
		U64 hash = k_fnvOffsetBasis;
		for (float value : values)
		{
			U32 bits;
			memcpy(&bits, &value, sizeof(bits));
			hash = (hash ^ bits) * k_fnvPrime;
		}
		return hash;
	}
}
