#pragma once

#include <Asset/AssetImport/MaterialAssetImporter.h>

#include <algorithm>
#include <optional>

// How the asset burner turns a source file's material values (OBJ, FBX) into ours. Plain values in and out, so the
// rules can be tested without Assimp; the burner reads the values from Assimp and passes them here.
namespace SourceMaterialRules
{
	// What a source declares about how see-through a material is. Each is absent when the source doesn't say.
	struct SourceTransparency
	{
		// Opacity, 1 solid (Assimp's AI_MATKEY_OPACITY: OBJ's d, or 1 - Tr)
		std::optional<float> Opacity;
		// Transparency, 1 fully see-through (FBX's TransparencyFactor, Assimp's $mat.transparencyfactor)
		std::optional<float> TransparencyFactor;
		// 3ds Max Physical Material transparency ($raw.3dsMax|Parameters|transparency): a physical transmission weight,
		// so clear glass is 1
		std::optional<float> MaxPhysicalTransparency;
	};

	// The first of these that the source declares:
	//  1. Opacity. OBJ always declares it, so OBJ never reaches the others (its Tr is already folded in).
	//  2. 1 - TransparencyFactor. 3ds Max writes it next to its physical value as a legacy approximation, and it's
	//     the better fit for a renderer without transmission: New Sponza's glass is 0.57 here (visible, tinted)
	//     but 1.0 physically (invisible).
	//  3. 1 - MaxPhysicalTransparency, for files with only the physical value.
	// Otherwise solid. Always within [0, 1].
	// #TODO(Transparency) Maya (Stingray/StandardSurface opacity, transmission) and Blender name theirs differently;
	// add them here as they turn up.
	inline float ChooseOpacity(const SourceTransparency& source)
	{
		float opacity = 1.0f;
		if (source.Opacity.has_value())
		{
			opacity = *source.Opacity;
		}
		else if (source.TransparencyFactor.has_value())
		{
			opacity = 1.0f - *source.TransparencyFactor;
		}
		else if (source.MaxPhysicalTransparency.has_value())
		{
			opacity = 1.0f - *source.MaxPhysicalTransparency;
		}
		return std::clamp(opacity, 0.0f, 1.0f);
	}

	// What a material's opacity mask means, from where the source connects it
	enum class EMaskKind
	{
		None,
		// Keep or cut, nothing between: OBJ's map_d, 3ds Max's cutout_map
		Cutout,
		// How much shows, blended: 3ds Max's transparency_map
		Coverage
	};

	// A cutout mask makes a cutout, whatever the opacity: the engine can't clip and fade the same material. A coverage
	// mask blends, even at full opacity, since the mask itself fades it. Without a mask, anything short of solid blends.
	inline MaterialAssetImporter::MaterialData::EBlendMode ChooseBlendMode(EMaskKind mask, float opacity)
	{
		using MaterialData = MaterialAssetImporter::MaterialData;
		switch (mask)
		{
		case EMaskKind::Cutout:
			return MaterialData::EBlendMode::Cutout;
		case EMaskKind::Coverage:
			return MaterialData::EBlendMode::Transparent;
		default:
			return opacity < 1.0f ? MaterialData::EBlendMode::Transparent : MaterialData::EBlendMode::Opaque;
		}
	}
}
