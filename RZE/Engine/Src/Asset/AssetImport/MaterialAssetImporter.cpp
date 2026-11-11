#include <Asset/AssetImport/MaterialAssetImporter.h>

#include <Asset/AssetImport/MaterialAssetWriter.h>

#include <Utils/Memory/ByteStream.h>
#include <Utils/Memory/ByteStreamUtils.h>

#include <cstring>

namespace
{
	// The newest version it reads; it reads every one before too
	constexpr uint16_t k_materialImporterVersion = 2;
	// The first with a tint
	constexpr uint16_t k_tintVersion = 2;
}

EBlendMode MaterialAssetImporter::GetLegacyBlendMode(U8 textureFlags)
{
	return (textureFlags & MaterialData::TEXTUREFLAG_OPACITY) ? EBlendMode::Cutout : EBlendMode::Opaque;
}

Vector3D MaterialAssetImporter::GetLegacyTint(U8 textureFlags)
{
	return (textureFlags & MaterialData::TEXTUREFLAG_ALBEDO) ? Vector3D(1.0f, 1.0f, 1.0f) : Vector3D(k_untexturedGrey, k_untexturedGrey, k_untexturedGrey);
}

bool MaterialAssetImporter::Import(const Filepath& filePath)
{
	mMaterialData = MaterialData();

	ByteStream byteStream(filePath.GetRelativePath());
	if (!byteStream.ReadFromFile(filePath) || byteStream.GetLength() < sizeof(size_t))
	{
		RZE_LOG_ERROR_ARGS("Failed to read material asset [%s].", filePath.GetRelativePath().c_str());
		return false;
	}

	// Version 0 has no header: it starts with its size, which never matches the magic
	U32 magic = 0;
	std::memcpy(&magic, byteStream.PeekBytes(), sizeof(U32));
	const bool hasHeader = magic == k_materialAssetMagic;
	uint16_t version = 0;
	if (hasHeader)
	{
		byteStream.PeekBytesAdvance(sizeof(U32));
		version = ByteStreamUtils::ReadType<uint16_t>(byteStream);
		if (version == 0 || version > k_materialImporterVersion)
		{
			RZE_LOG_ERROR_ARGS("Material asset [%s] has version %u, expected 1 to %u.", filePath.GetRelativePath().c_str(), static_cast<U32>(version), static_cast<U32>(k_materialImporterVersion));
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
		? ByteStreamUtils::ReadType<EBlendMode>(byteStream)
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

	// Each path takes at least its size, so a count that can't fit in what's left is wrong, not just short. Checked
	// before the loop, which it bounds.
	const size_t textureCount = ByteStreamUtils::ReadType<size_t>(byteStream);
	if (byteStream.HasFailed() || textureCount > byteStream.GetRemainingLength() / sizeof(size_t))
	{
		return FailImport(filePath);
	}

	for (size_t i = 0; i < textureCount; ++i)
	{
		mMaterialData.TexturePaths.push_back(ByteStreamUtils::ReadString(byteStream));
	}
	// A path cut short
	if (byteStream.HasFailed())
	{
		return FailImport(filePath);
	}

	return true;
}

bool MaterialAssetImporter::FailImport(const Filepath& filePath)
{
	RZE_LOG_ERROR_ARGS("Material asset [%s] is shorter than its contents say, or holds a size that can't be right.", filePath.GetRelativePath().c_str());
	mMaterialData = MaterialData();
	return false;
}
