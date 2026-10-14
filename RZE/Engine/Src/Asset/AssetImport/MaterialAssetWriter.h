#pragma once

#include <Asset/AssetImport/AssetWriter.h>
#include <Asset/AssetImport/MaterialAssetImporter.h>

#include <Utils/Platform/Filepath.h>

// Version 1 onwards: a .materialasset starts with this, then the version. Version 0 files, which started with their
// size instead, can't begin with these bytes (they'd have to be over a gigabyte).
constexpr U32 k_materialAssetMagic = 0x544D5A52; // "RZMT"

// Writes a .materialasset. The format lives here and in MaterialAssetImporter only:
//   U32 magic, U16 version,
//   name (size_t length + bytes), MaterialProperties, U8 texture flags, U8 blend mode,
//   size_t texture count, then each texture path (size_t length + bytes)
class MaterialAssetWriter : public AssetWriter
{
public:
	MaterialAssetWriter() = default;

	void SetMaterialData(const MaterialAssetImporter::MaterialData& materialData);
	void SetOutputPath(const Filepath& outputPath);

	void Write() override;

private:
	MaterialAssetImporter::MaterialData m_materialData;
	Filepath m_outputPath;
};
