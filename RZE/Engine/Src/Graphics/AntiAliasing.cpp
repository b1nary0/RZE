#include <Graphics/AntiAliasing.h>

#include <Utils/EnumNames.h>

#include <algorithm>
#include <initializer_list>

namespace
{
	// In EAntiAliasingMethod's order
	constexpr const char* k_methodNames[] = { "Off", "FXAA" };
	static_assert(sizeof(k_methodNames) / sizeof(k_methodNames[0]) == static_cast<size_t>(EAntiAliasingMethod::Count), "A name for every method");

	// In EAntiAliasingQuality's order
	constexpr const char* k_qualityNames[] = { "Low", "Medium", "High" };
	static_assert(sizeof(k_qualityNames) / sizeof(k_qualityNames[0]) == static_cast<size_t>(EAntiAliasingQuality::Count), "A name for every quality");

	FXAASettings MakeFXAASettings(float edgeThreshold, float edgeThresholdMin, float subpixel, std::initializer_list<float> searchSteps)
	{
		AssertExpr(searchSteps.size() <= FXAASettings::k_maxSearchSteps);

		FXAASettings settings;
		settings.EdgeThreshold = edgeThreshold;
		settings.EdgeThresholdMin = edgeThresholdMin;
		settings.Subpixel = subpixel;
		settings.SearchStepCount = static_cast<int>(searchSteps.size());
		std::copy(searchSteps.begin(), searchSteps.end(), settings.SearchSteps.begin());
		return settings;
	}
}

float FXAASettings::GetSearchLength() const
{
	float length = 0.0f;
	for (int step = 0; step < SearchStepCount; ++step)
	{
		length += SearchSteps[step];
	}
	return length;
}

FXAASettings GetFXAASettings(EAntiAliasingQuality quality)
{
	// FXAA 3.11's presets and recommended values: higher qualities treat fainter contrast as an edge, search further
	// along it in finer steps, and soften more sub-pixel detail
	switch (quality)
	{
	case EAntiAliasingQuality::Low:
		return MakeFXAASettings(0.250f, 0.0833f, 0.25f, { 1.5f, 3.0f, 12.0f });
	case EAntiAliasingQuality::High:
		return MakeFXAASettings(0.125f, 0.0312f, 0.75f, { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.5f, 2.0f, 2.0f, 2.0f, 2.0f, 4.0f, 8.0f });
	case EAntiAliasingQuality::Medium:
	default:
		return MakeFXAASettings(0.166f, 0.0625f, 0.50f, { 1.0f, 1.5f, 2.0f, 4.0f, 12.0f });
	}
}

const EnumNames& GetAntiAliasingMethodNames()
{
	static const EnumNames names(k_methodNames);
	return names;
}

const char* GetAntiAliasingMethodName(EAntiAliasingMethod method)
{
	return GetAntiAliasingMethodNames().GetName(static_cast<size_t>(method));
}

std::optional<EAntiAliasingMethod> ParseAntiAliasingMethod(const std::string& name)
{
	const std::optional<size_t> method = GetAntiAliasingMethodNames().FindIgnoringCase(name);
	return method ? std::optional<EAntiAliasingMethod>(static_cast<EAntiAliasingMethod>(*method)) : std::nullopt;
}

const EnumNames& GetAntiAliasingQualityNames()
{
	static const EnumNames names(k_qualityNames);
	return names;
}

const char* GetAntiAliasingQualityName(EAntiAliasingQuality quality)
{
	return GetAntiAliasingQualityNames().GetName(static_cast<size_t>(quality));
}

std::optional<EAntiAliasingQuality> ParseAntiAliasingQuality(const std::string& name)
{
	const std::optional<size_t> quality = GetAntiAliasingQualityNames().FindIgnoringCase(name);
	return quality ? std::optional<EAntiAliasingQuality>(static_cast<EAntiAliasingQuality>(*quality)) : std::nullopt;
}
