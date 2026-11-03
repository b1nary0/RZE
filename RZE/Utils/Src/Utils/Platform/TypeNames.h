#pragma once

#include <string>
#include <typeinfo>

// Types' names as the code spells them. type_info::name() is each compiler's own: MSVC's is "class RenderComponent",
// clang's and GCC's the mangled "15RenderComponent". Per platform: Utils/Platform/<platform>/TypeNames<platform>.cpp.
namespace TypeNames
{
	// type's name with its namespaces, e.g. "RenderComponent" or "Rendering::GLDevice", without "class " or "struct "
	std::string Get(const std::type_info& type);
}
