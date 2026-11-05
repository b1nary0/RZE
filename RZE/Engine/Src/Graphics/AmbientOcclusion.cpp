#include <Graphics/AmbientOcclusion.h>

#include <Utils/StringUtils.h>

AmbientOcclusionSettings GetAmbientOcclusionSettings(EAmbientOcclusion level)
{
	// XeGTAO's presets: more slices see more of the hemisphere, more steps find thinner occluders. Without TAA the fixed
	// noise is all that's averaged, so Medium and High blur twice.
	switch (level)
	{
	case EAmbientOcclusion::Low:
		return { 1, 2, 1 };
	case EAmbientOcclusion::Medium:
		return { 2, 2, 2 };
	case EAmbientOcclusion::High:
		return { 3, 3, 2 };
	default:
		return {};
	}
}

const char* GetAmbientOcclusionName(EAmbientOcclusion level)
{
	switch (level)
	{
	case EAmbientOcclusion::Off:
		return "Off";
	case EAmbientOcclusion::Low:
		return "Low";
	case EAmbientOcclusion::Medium:
		return "Medium";
	case EAmbientOcclusion::High:
		return "High";
	default:
		return "";
	}
}

std::optional<EAmbientOcclusion> ParseAmbientOcclusion(const std::string& name)
{
	for (int level = 0; level < static_cast<int>(EAmbientOcclusion::Count); ++level)
	{
		if (StringUtils::EqualsIgnoringCase(name, GetAmbientOcclusionName(static_cast<EAmbientOcclusion>(level))))
		{
			return static_cast<EAmbientOcclusion>(level);
		}
	}
	return std::nullopt;
}
