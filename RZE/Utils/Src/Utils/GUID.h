#pragma once

#include <Boost/uuid/uuid.hpp>
#include <Boost/uuid/uuid_io.hpp>
#include <Boost/uuid/uuid_generators.hpp>

typedef boost::uuids::uuid GUID;

namespace GUIDHelper
{
	inline GUID GenerateGUID()
	{
		return boost::uuids::random_generator()();
	}
}