#include <StdAfx.h>
#include <Asset/AssetImport/MaterialAssetImporter.h>

#include <Asset/AssetImport/MaterialAssetWriter.h>

#include <Utils/Memory/ByteStream.h>
#include <Utils/Memory/ByteStreamUtils.h>

namespace
{
	// The newest version it reads; it reads every one before too
	constexpr uint16_t k_materialImporterVersion = 2;
	// The first with a tint
	constexpr uint16_t k_tintVersion = 2;
}

MaterialAssetImporter::MaterialData::EBlendMode MaterialAssetImporter::GetLegacyBlendMode(U8 textureFlags)
{
	return (textureFlags & MaterialData::TEXTUREFLAG_OPACITY) ? MaterialData::EBlendMode::Cutout : MaterialData::EBlendMode::Opaque;
}

Vector3D MaterialAssetImporter::GetLegacyTint(U8 textureFlags)
{
	return (textureFlags & MaterialData::TEXTUREFLAG_ALBEDO) ? Vector3D(1.0f, 1.0f, 1.0f) : Vector3D(k_untexturedGrey, k_untexturedGrey, k_untexturedGrey);
}

const char* MaterialAssetImporter::GetBlendModeName(MaterialData::EBlendMode blendMode)
{
	switch (blendMode)
	{
	case MaterialData::EBlendMode::Opaque:
		return "Opaque";
	case MaterialData::EBlendMode::Cutout:
		return "Cutout";
	case MaterialData::EBlendMode::Transparent:
		return "Transparent";
	case MaterialData::EBlendMode::Premultiplied:
		return "Premultiplied";
	case MaterialData::EBlendMode::Additive:
		return "Additive";
	case MaterialData::EBlendMode::Multiply:
		return "Multiply";
	default:
		return "Unknown";
	}
}

bool MaterialAssetImporter::IsBlended(MaterialData::EBlendMode blendMode)
{
	return blendMode != MaterialData::EBlendMode::Opaque && blendMode != MaterialData::EBlendMode::Cutout;
}

bool MaterialAssetImporter::Import(const Filepath& filePath)
{
	// @TODO Validate contents; currently only a missing/empty file is detected.
	ByteStream byteStream(filePath.GetRelativePath());
	if (!byteStream.ReadFromFile(filePath) || byteStream.GetLength() < sizeof(size_t))
	{
		RZE_LOG_ARGS("Failed to read material asset [%s].", filePath.GetRelativePath().c_str());
		return false;
	}

	// Version 0 has no header: it starts with its size, which never matches the magic
	const bool hasHeader = *reinterpret_cast<const U32*>(byteStream.PeekBytes()) == k_materialAssetMagic;
	uint16_t version = 0;
	if (hasHeader)
	{
		byteStream.PeekBytesAdvance(sizeof(U32));
		version = ByteStreamUtils::ReadType<uint16_t>(byteStream);
		if (version == 0 || version > k_materialImporterVersion)
		{
			RZE_LOG_ARGS("Material asset [%s] has version %u, expected 1 to %u.", filePath.GetRelativePath().c_str(), static_cast<U32>(version), static_cast<U32>(k_materialImporterVersion));
			return false;
		}
	}
	else
	{
		byteStream.PeekBytesAdvance(sizeof(size_t)); // bufSize
	}

	mMaterialData.MaterialName = ByteStreamUtils::ReadString(byteStream);
	mMaterialData.Properties = ByteStreamUtils::ReadType<MaterialData::MaterialProperties>(byteStream);
	mMaterialData.TextureFlags = ByteStreamUtils::ReadType<U8>(byteStream);
	mMaterialData.BlendMode = hasHeader
		? ByteStreamUtils::ReadType<MaterialData::EBlendMode>(byteStream)
		: GetLegacyBlendMode(mMaterialData.TextureFlags);
	if (version >= k_tintVersion)
	{
		const float red = ByteStreamUtils::ReadType<float>(byteStream);
		const float green = ByteStreamUtils::ReadType<float>(byteStream);
		const float blue = ByteStreamUtils::ReadType<float>(byteStream);
		mMaterialData.Tint = Vector3D(red, green, blue);
	}
	else
	{
		mMaterialData.Tint = GetLegacyTint(mMaterialData.TextureFlags);
	}

	const size_t textureCount = *reinterpret_cast<size_t*>(byteStream.PeekBytesAdvance(sizeof(size_t)));
	for (size_t i = 0; i < textureCount; ++i)
	{
		mMaterialData.TexturePaths.push_back(ByteStreamUtils::ReadString(byteStream));
	}

	return true;
}
