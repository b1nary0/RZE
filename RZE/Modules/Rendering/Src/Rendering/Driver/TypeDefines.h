#pragma once

#include <Utils/PrimitiveDefs.h>

namespace Rendering
{
	enum class EDataFormat : U8
	{
		R32G32_FLOAT = 0,
		R32G32B32_FLOAT,
		R32G32B32A32_FLOAT,
		// One unsigned integer, e.g. an index per instance
		R32_UINT
	};

	enum class EDataClassification : U8
	{
		PER_INSTANCE = 0,
		PER_VERTEX
	};
}