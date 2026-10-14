#pragma once

#include <Asset/AssetImport/AssetImporter.h>

#include <Utils/PrimitiveDefs.h>

#include <string>
#include <vector>

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
			TEXTUREFLAG_OPACITY = 1 << 3,
			TEXTUREFLAG_HEIGHT = 1 << 4,
			TEXTUREFLAG_OPACITY_IN_ALPHA = 1 << 5, // Opacity texture is the diffuse texture; cutout is in its alpha
			TEXTUREFLAG_INVERT_OPACITY = 1 << 6, // Opacity texture's white means see-through (3ds Max transparency_map)
			TEXTUREFLAG_ALL = TEXTUREFLAG_ALBEDO | TEXTUREFLAG_SPECULAR | TEXTUREFLAG_NORMAL
		};

		// How the material covers what's behind it (Docs/Plans/Transparency.md). Stored as a byte; append new modes.
		enum class EBlendMode : U8
		{
			Opaque,      // Hides what's behind it
			Cutout,      // Opaque where its opacity mask passes AlphaCutoff, absent elsewhere
			Transparent, // See-through by Properties.Opacity: src * a + dst * (1 - a)
			Premultiplied, // For colour already multiplied by its alpha: src + dst * (1 - a)
			Additive,    // Adds its colour, scaled by opacity, to what's behind (glows, light shafts): src * a + dst
			Multiply     // Darkens what's behind by its colour (grime): src * dst. #TODO(Transparency) ignores opacity
		};

		std::string MaterialName;
		struct MaterialProperties
		{
			float Shininess = 0.0f;;
			// 1 is solid. Only a Transparent material is see-through by it.
			float Opacity = 1.0f;
		} Properties;
		// #TODO These flags inform which texture types are used by this material
		// probably not the best implementation of this but i think the asset infrastructure
		// needs to mature.
		U8 TextureFlags = TEXTUREFLAG_NONE;
		EBlendMode BlendMode = EBlendMode::Opaque;
		std::vector<std::string> TexturePaths;
	};

	// What a material made before blend modes existed (version 0) is: Cutout with an opacity mask, otherwise Opaque
	static MaterialData::EBlendMode GetLegacyBlendMode(U8 textureFlags);
	static const char* GetBlendModeName(MaterialData::EBlendMode blendMode);
	// Blends with what's behind it, so it's drawn after everything opaque, sorted, and casts no shadow
	static bool IsBlended(MaterialData::EBlendMode blendMode);

public:
	MaterialAssetImporter() = default;
	virtual ~MaterialAssetImporter() = default;

public:
	virtual bool Import(const Filepath& filePath) override;

public:
	MaterialData GetMaterialData() { return mMaterialData; }

private:
	std::string ReadName(ByteStream& byteStream);
	MaterialData::MaterialProperties ReadMaterialProperties(ByteStream& byteStream);
	U8 ReadTextureFlags(ByteStream& byteStream);

private:
	MaterialData mMaterialData;
};