#include <StdAfx.h>
#include <Graphics/MaterialValues.h>

#include <algorithm>

MaterialValues MaterialValues::FromMaterialData(const MaterialAssetImporter::MaterialData& materialData)
{
	using MaterialData = MaterialAssetImporter::MaterialData;

	MaterialValues values;
	values.BlendMode = materialData.BlendMode;
	values.Opacity = materialData.Properties.Opacity;
	values.Shininess = materialData.Properties.Shininess;
	values.Tint = materialData.Tint;

	// An empty path stays an invalid Filepath: Filepath("") would resolve to the root folder
	const size_t textureCount = std::min(materialData.TexturePaths.size(), k_textureSlotCount);
	for (size_t textureSlot = 0; textureSlot < textureCount; ++textureSlot)
	{
		const std::string& texturePath = materialData.TexturePaths[textureSlot];
		if (!texturePath.empty())
		{
			values.Textures[textureSlot] = Filepath(texturePath);
		}
	}

	values.InvertOpacityMask = (materialData.TextureFlags & MaterialData::TEXTUREFLAG_INVERT_OPACITY) != 0;
	values.OpacityMaskInAlpha = (materialData.TextureFlags & MaterialData::TEXTUREFLAG_OPACITY_IN_ALPHA) != 0;
	return values;
}
