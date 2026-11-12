#include <Utils/StringUtils.h>

#include <cmath>
#include <cstdio>
#include <cstring>

void StringUtils::CopyTruncated(char* destination, size_t destinationSize, std::string_view source)
{
	if (destinationSize == 0)
	{
		return;
	}

	const size_t count = std::min(source.size(), destinationSize - 1);
	std::memcpy(destination, source.data(), count);
	destination[count] = '\0';
}

std::string StringUtils::ExtractFileNameFromPath(const std::string& filePath)
{
	const size_t index = filePath.find_last_of("/\\");
	if (index == std::string::npos)
	{
		return filePath;
	}
	return filePath.substr(index + 1);
}

std::string StringUtils::Trim(const std::string& text)
{
	const size_t first = text.find_first_not_of(" \t");
	const size_t last = text.find_last_not_of(" \t");
	return first == std::string::npos ? std::string() : text.substr(first, last - first + 1);
}

std::string StringUtils::FormatCount(double count)
{
	constexpr const char* k_suffixes[] = { "", "K", "M", "G", "T" };
	constexpr size_t k_lastSuffix = sizeof(k_suffixes) / sizeof(k_suffixes[0]) - 1;

	const char* sign = count < 0.0 ? "-" : "";
	double scaled = std::fabs(count);
	// Up a unit at whatever would round to 1000 in this one, so 999.5 is "1.00K", not "1000"
	size_t unit = 0;
	while (scaled >= 999.5 && unit < k_lastSuffix)
	{
		scaled /= 1000.0;
		++unit;
	}

	char text[32];
	if (unit == 0)
	{
		snprintf(text, sizeof(text), "%s%.0f", sign, scaled);
		return text;
	}
	// Three significant figures, deciding the decimals by what the value rounds to: 9.995 is "10.0", not "10.00"
	const int decimals = scaled < 9.995 ? 2 : (scaled < 99.95 ? 1 : 0);
	snprintf(text, sizeof(text), "%s%.*f%s", sign, decimals, scaled, k_suffixes[unit]);
	return text;
}
