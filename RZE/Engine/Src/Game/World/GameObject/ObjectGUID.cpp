#include <StdAfx.h>
#include <Game/World/GameObject/ObjectGUID.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>

namespace ObjectGUIDs
{
	ObjectGUID Generate()
	{
		// One generator per thread, seeded from the OS, so threads never share state
		thread_local std::mt19937_64 s_generator(std::random_device{}());

		ObjectGUID guid = k_invalid;
		while (guid == k_invalid)
		{
			guid = s_generator();
		}
		return guid;
	}

	std::string ToString(ObjectGUID guid)
	{
		char text[17];
		snprintf(text, sizeof(text), "%016llx", static_cast<unsigned long long>(guid));
		return std::string(text);
	}

	ObjectGUID FromString(const char* text)
	{
		if (text == nullptr || strlen(text) != 16)
		{
			return k_invalid;
		}

		char* end = nullptr;
		const unsigned long long value = strtoull(text, &end, 16);
		return end == text + 16 ? static_cast<ObjectGUID>(value) : k_invalid;
	}
}
