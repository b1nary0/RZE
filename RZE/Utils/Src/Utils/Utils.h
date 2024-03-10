#pragma once

#include <Utils/DebugUtils/Debug.h>

#include <vector>

namespace ContainerUtils
{
	template <typename ContainerType>
	void VectorEraseBack(ContainerType& vector, typename ContainerType::iterator it)
	{
		AssertExpr(vector.size() > 0);

		size_t idx = std::distance(vector.begin(), it);
		vector[idx] = vector.back();
		vector.back() = *it;
		vector.pop_back();
	}
}