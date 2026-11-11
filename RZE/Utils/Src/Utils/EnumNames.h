#pragma once

#include <cstddef>
#include <optional>
#include <string>

// The names of an enum's values, in value order (0, 1, 2, ...), as settings show and read them: Engine.ini values, the
// editor's options. The names are string literals in a table the enum's .cpp owns, e.g.
//	constexpr const char* k_names[] = { "Off", "Low", "Medium", "High" };
//	const EnumNames& GetAmbientOcclusionNames() { static const EnumNames names(k_names); return names; }
class EnumNames
{
public:
	template <size_t Count>
	explicit constexpr EnumNames(const char* const (&names)[Count])
		: m_names(names)
		, m_count(Count) {}

public:
	size_t GetCount() const { return m_count; }

	// The value's name, or "" for a value outside the table
	const char* GetName(size_t value) const;

	// The value with this name, ignoring case (ASCII); none if no value has it
	std::optional<size_t> FindIgnoringCase(const std::string& name) const;

	// Every name, for a message saying which are allowed: "Off, Low, Medium or High"
	std::string ListAlternatives() const;

private:
	const char* const* m_names;
	size_t m_count;
};
