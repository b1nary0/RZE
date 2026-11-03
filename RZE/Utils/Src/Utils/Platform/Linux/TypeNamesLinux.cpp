#include <StdAfx.h>
#include <Utils/Platform/TypeNames.h>

#include <cxxabi.h>

#include <cstdlib>
#include <memory>

namespace TypeNames
{
	std::string Get(const std::type_info& type)
	{
		// The Itanium C++ ABI's mangled name, which the runtime turns back into the source's
		int status = 0;
		const std::unique_ptr<char, decltype(&std::free)> demangled(abi::__cxa_demangle(type.name(), nullptr, nullptr, &status), &std::free);
		return status == 0 ? std::string(demangled.get()) : std::string(type.name());
	}
}
