#pragma once

#include <Asset/AssetImport/AssetWriter.h>

#include <Utils/PrimitiveDefs.h>

#include <string>
#include <vector>

struct MaterialData
{
	enum ETextureFlags
	{
		TEXTUREFLAG_NONE = 0,
		TEXTUREFLAG_ALBEDO = 1 << 0,
		TEXTUREFLAG_SPECULAR = 1 << 1,
		TEXTUREFLAG_NORMAL = 1 << 2,
		TEXTUREFLAG_ALL = TEXTUREFLAG_ALBEDO | TEXTUREFLAG_SPECULAR | TEXTUREFLAG_NORMAL
	};

	std::string MaterialName;
	struct MaterialProperties
	{
		float Shininess = 0.0f;;
		float Opacity = 1.0f;
	} Properties;

	// #TODO These flags inform which texture types are used by this material
	// probably not the best implementation of this but i think the asset infrastructure
	// needs to mature.
	U8 TextureFlags = TEXTUREFLAG_NONE;
	std::vector<std::string> TexturePaths;
};

struct MaterialAssetFileHeader
{
	uint16_t AssetVersion;
	size_t BufSize;
};

class MaterialAssetWriter : public AssetWriter
{
public:
	MaterialAssetWriter() = default;

	void SetMaterialData(const MaterialData& materialData);
	void SetAssetName(const std::string& assetName);

	void Write() override;
	
private:
	MaterialData m_materialData;
	std::string m_assetName;
};