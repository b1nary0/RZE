#pragma once

#include <Asset/AssetImport/AssetImporter.h>

#include <Graphics/MaterialDefines.h>

#include <Utils/Math/Vector3D.h>
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

		std::string MaterialName;
		struct MaterialProperties
		{
			float Shininess = 0.0f;
			// 1 is solid. Only a Transparent material (EBlendMode) is see-through by it.
			float Opacity = 1.0f;
		} Properties;
		// #TODO These flags inform which texture types are used by this material
		// probably not the best implementation of this but i think the asset infrastructure
		// needs to mature.
		U8 TextureFlags = TEXTUREFLAG_NONE;
		EBlendMode BlendMode = EBlendMode::Opaque;
		// Multiplies the diffuse texture's colour, or is the colour without one; sRGB, each in [0, 1]
		Vector3D Tint = Vector3D(1.0f, 1.0f, 1.0f);
		std::vector<std::string> TexturePaths;
	};

	// The grey an untextured material drew with before tints existed, and the burner's tint for a source that gives
	// no colour; sRGB
	static constexpr float k_untexturedGrey = 0.5f;

	// What a material made before blend modes existed (version 0) is: Cutout with an opacity mask, otherwise Opaque
	static EBlendMode GetLegacyBlendMode(U8 textureFlags);
	// What a material made before tints existed (versions 0 and 1) drew with: its diffuse texture as it is (white),
	// or k_untexturedGrey without one
	static Vector3D GetLegacyTint(U8 textureFlags);

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