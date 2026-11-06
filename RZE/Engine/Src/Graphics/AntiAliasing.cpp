#include <Graphics/AntiAliasing.h>

#include <Utils/EnumNames.h>

#include <algorithm>
#include <initializer_list>

namespace
{
	// In EAntiAliasing's order
	constexpr const char* k_names[] = { "Off", "Low", "Medium", "High" };
	static_assert(sizeof(k_names) / sizeof(k_names[0]) == static_cast<size_t>(EAntiAliasing::Count), "A name for every level");

	AntiAliasingSettings MakeSettings(float edgeThreshold, float edgeThresholdMin, float subpixel, std::initializer_list<float> searchSteps)
	{
		AssertExpr(searchSteps.size() <= AntiAliasingSettings::k_maxSearchSteps);

		AntiAliasingSettings settings;
		settings.EdgeThreshold = edgeThreshold;
		settings.EdgeThresholdMin = edgeThresholdMin;
		settings.Subpixel = subpixel;
		settings.SearchStepCount = static_cast<int>(searchSteps.size());
		std::copy(searchSteps.begin(), searchSteps.end(), settings.SearchSteps.begin());
		return settings;
	}
}

float AntiAliasingSettings::GetSearchLength() const
{
	float length = 0.0f;
	for (int step = 0; step < SearchStepCount; ++step)
	{
		length += SearchSteps[step];
	}
	return length;
}

AntiAliasingSettings GetAntiAliasingSettings(EAntiAliasing level)
{
	// FXAA 3.11's presets and recommended values: higher levels treat fainter contrast as an edge, search further
	// along it in finer steps, and soften more sub-pixel detail
	switch (level)
	{
	case EAntiAliasing::Low:
		return MakeSettings(0.250f, 0.0833f, 0.25f, { 1.5f, 3.0f, 12.0f });
	case EAntiAliasing::Medium:
		return MakeSettings(0.166f, 0.0625f, 0.50f, { 1.0f, 1.5f, 2.0f, 4.0f, 12.0f });
	case EAntiAliasing::High:
		return MakeSettings(0.125f, 0.0312f, 0.75f, { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.5f, 2.0f, 2.0f, 2.0f, 2.0f, 4.0f, 8.0f });
	default:
		return {};
	}
}

const EnumNames& GetAntiAliasingNames()
{
	static const EnumNames names(k_names);
	return names;
}

const char* GetAntiAliasingName(EAntiAliasing level)
{
	return GetAntiAliasingNames().GetName(static_cast<size_t>(level));
}

std::optional<EAntiAliasing> ParseAntiAliasing(const std::string& name)
{
	const std::optional<size_t> level = GetAntiAliasingNames().FindIgnoringCase(name);
	return level ? std::optional<EAntiAliasing>(static_cast<EAntiAliasing>(*level)) : std::nullopt;
}
