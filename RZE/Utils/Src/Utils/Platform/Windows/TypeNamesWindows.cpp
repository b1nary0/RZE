#include <StdAfx.h>
#include <Utils/Platform/TypeNames.h>

namespace TypeNames
{
	std::string Get(const std::type_info& type)
	{
		// MSVC's names start with what the type is declared as ("class ", "struct ")
		const std::string name = type.name();
		return name.substr(name.find_first_of(' ') + 1, name.size());
	}
}
