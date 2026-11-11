#include <Utils/EnumNames.h>

#include <Utils/StringUtils.h>

const char* EnumNames::GetName(size_t value) const
{
	return value < m_count ? m_names[value] : "";
}

std::optional<size_t> EnumNames::FindIgnoringCase(const std::string& name) const
{
	for (size_t value = 0; value < m_count; ++value)
	{
		if (StringUtils::EqualsIgnoringCase(name, m_names[value]))
		{
			return value;
		}
	}
	return std::nullopt;
}

std::string EnumNames::ListAlternatives() const
{
	std::string list;
	for (size_t value = 0; value < m_count; ++value)
	{
		if (value > 0)
		{
			list += value + 1 == m_count ? " or " : ", ";
		}
		list += m_names[value];
	}
	return list;
}
