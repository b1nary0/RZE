#pragma once

#include <Asset/AssetImport/AssetImporter.h>

class ByteStream;

class MaterialAssetImporter : public AssetImporter
{
public:
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
		U8 TextureFlags;
		std::vector<std::string> TexturePaths;
	};

public:
	MaterialAssetImporter() = default;
	virtual ~MaterialAssetImporter() = default;

public:
	virtual bool Import(const FilePath& filePath) override;

public:
	MaterialData GetMaterialData() { return mMaterialData; }

private:
	std::string ReadName(ByteStream& byteStream);
	MaterialData::MaterialProperties ReadMaterialProperties(ByteStream& byteStream);
	U8 ReadTextureFlags(ByteStream& byteStream);

private:
	MaterialData mMaterialData;
};