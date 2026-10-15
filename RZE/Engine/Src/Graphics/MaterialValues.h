#pragma once

#include <Asset/AssetImport/MaterialAssetImporter.h>

#include <Utils/Platform/Filepath.h>

#include <array>

// Everything about a material that can be edited: what MaterialInstance::SetValues turns into its shader, textures
// and GPU parameters
struct MaterialValues
{
	using EBlendMode = MaterialAssetImporter::MaterialData::EBlendMode;

	// One per MaterialInstance::TextureSlot (diffuse, specular, normal, opacity, height); checked in Material.h
	static constexpr size_t k_textureSlotCount = 5;

	// A material asset's values, as MaterialAssetImporter reads them
	static MaterialValues FromMaterialData(const MaterialAssetImporter::MaterialData& materialData);

	EBlendMode BlendMode = EBlendMode::Opaque;
	// 1 is solid. Only a blended material is see-through by it.
	float Opacity = 1.0f;
	float Shininess = 1.0f;
	// Indexed by MaterialInstance::TextureSlot; a default (invalid) Filepath for an empty slot
	std::array<Filepath, k_textureSlotCount> Textures;
	// The opacity texture's white means see-through rather than solid
	bool InvertOpacityMask = false;
	// The opacity mask is in the opacity texture's alpha rather than its red (its opacity texture is its diffuse one)
	bool OpacityMaskInAlpha = false;
};
