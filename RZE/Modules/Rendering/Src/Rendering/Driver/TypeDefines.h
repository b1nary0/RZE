#pragma once

#include <Utils/PrimitiveDefs.h>

namespace Rendering
{
	enum class EDataFormat : U8
	{
		R32G32_FLOAT = 0,
		R32G32B32_FLOAT
	};

	enum class EDataClassification : U8
	{
		PER_INSTANCE = 0,
		PER_VERTEX
	};
}