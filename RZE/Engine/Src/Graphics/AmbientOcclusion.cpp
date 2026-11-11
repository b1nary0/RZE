#include <Graphics/AmbientOcclusion.h>

#include <Utils/EnumNames.h>

namespace
{
	// In EAmbientOcclusion's order
	constexpr const char* k_names[] = { "Off", "Low", "Medium", "High" };
	static_assert(sizeof(k_names) / sizeof(k_names[0]) == static_cast<size_t>(EAmbientOcclusion::Count), "A name for every level");
}

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

const EnumNames& GetAmbientOcclusionNames()
{
	static const EnumNames names(k_names);
	return names;
}

const char* GetAmbientOcclusionName(EAmbientOcclusion level)
{
	return GetAmbientOcclusionNames().GetName(static_cast<size_t>(level));
}

std::optional<EAmbientOcclusion> ParseAmbientOcclusion(const std::string& name)
{
	const std::optional<size_t> level = GetAmbientOcclusionNames().FindIgnoringCase(name);
	return level ? std::optional<EAmbientOcclusion>(static_cast<EAmbientOcclusion>(*level)) : std::nullopt;
}
