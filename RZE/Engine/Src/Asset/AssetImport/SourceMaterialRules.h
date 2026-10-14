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

	// A mask wins: it's a cutout even when the material is also partly see-through, since the engine can't do both
	// yet (#TODO(Transparency) Part 2 makes the mask blend instead, Docs/Plans/Transparency.md). Otherwise anything
	// short of solid is Transparent.
	inline MaterialAssetImporter::MaterialData::EBlendMode ChooseBlendMode(U8 textureFlags, float opacity)
	{
		using MaterialData = MaterialAssetImporter::MaterialData;
		if (textureFlags & MaterialData::TEXTUREFLAG_OPACITY)
		{
			return MaterialData::EBlendMode::Cutout;
		}
		return opacity < 1.0f ? MaterialData::EBlendMode::Transparent : MaterialData::EBlendMode::Opaque;
	}
}
