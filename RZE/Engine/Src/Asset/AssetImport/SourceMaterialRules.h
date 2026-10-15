#pragma once

#include <Asset/AssetImport/MaterialAssetImporter.h>

#include <optional>

// How the asset burner turns a source file's material values (OBJ, FBX) into ours. Plain values in and out, so the
// rules can be tested without Assimp; the burner reads the values from Assimp and passes them here. In the engine
// library so the burner and the unit tests both link them.
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
	float ChooseOpacity(const SourceTransparency& source);

	// What a material's opacity mask means, from where the source connects it
	enum class EMaskKind
	{
		None,
		// Keep or cut, nothing between: OBJ's map_d, 3ds Max's cutout_map
		Cutout,
		// How much shows, blended: 3ds Max's transparency_map
		Coverage
	};

	// What a source declares about a material's colour. Each is absent when the source doesn't say.
	struct SourceColour
	{
		// Assimp's AI_MATKEY_COLOR_DIFFUSE: OBJ's Kd, FBX's DiffuseColor
		std::optional<Vector3D> Diffuse;
		// 3ds Max Physical Material transmission colour ($raw.3dsMax|Parameters|trans_color), and how much of the
		// surface transmits ($raw.3dsMax|Parameters|transparency, as in SourceTransparency)
		std::optional<Vector3D> MaxTransmissionColour;
		std::optional<float> MaxPhysicalTransparency;
	};

	// The tint (MaterialData::Tint):
	//  - White with a diffuse texture, which carries the colour; sources often keep a default grey beside one
	//    (Sponza's Kd 0.588), which would only darken it.
	//  - Otherwise the diffuse colour (or MaterialAssetImporter::k_untexturedGrey without one), mixed toward 3ds Max's
	//    transmission colour by how much transmits: a renderer without transmission sees glass by it. New Sponza's
	//    glass has a black diffuse colour and transmits fully, light blue.
	// Always within [0, 1].
	Vector3D ChooseTint(const SourceColour& source, bool hasDiffuseTexture);

	// A cutout mask makes a cutout, whatever the opacity: the engine can't clip and fade the same material. A coverage
	// mask blends, even at full opacity, since the mask itself fades it. Without a mask, anything short of solid blends.
	MaterialAssetImporter::MaterialData::EBlendMode ChooseBlendMode(EMaskKind mask, float opacity);
}
