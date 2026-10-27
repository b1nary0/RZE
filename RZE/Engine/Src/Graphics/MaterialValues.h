#pragma once

#include <Asset/AssetImport/MaterialAssetImporter.h>

#include <Graphics/MaterialDefines.h>

#include <Utils/Math/Vector3D.h>
#include <Utils/Platform/Filepath.h>

#include <array>

// Everything about a material that can be edited: what MaterialInstance::SetValues turns into its shader, textures
// and GPU parameters
struct MaterialValues
{
	// A material asset's values, as MaterialAssetImporter reads them
	static MaterialValues FromMaterialData(const MaterialAssetImporter::MaterialData& materialData);

	EBlendMode BlendMode = EBlendMode::Opaque;
	// 1 is solid. Only a blended material is see-through by it.
	float Opacity = 1.0f;
	float Shininess = 1.0f;
	// Multiplies the diffuse texture's colour, or is the colour without one; sRGB, each in [0, 1]
	Vector3D Tint = Vector3D(1.0f, 1.0f, 1.0f);
	// Indexed by ETextureSlot; a default (invalid) Filepath for an empty slot
	std::array<Filepath, TEXTURE_SLOT_COUNT> Textures;
	// The opacity texture's white means see-through rather than solid
	bool InvertOpacityMask = false;
	// The opacity mask is in the opacity texture's alpha rather than its red (its opacity texture is its diffuse one)
	bool OpacityMaskInAlpha = false;
};
