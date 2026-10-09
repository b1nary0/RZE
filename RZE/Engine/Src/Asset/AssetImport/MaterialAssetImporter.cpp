#include <StdAfx.h>
#include <Asset/AssetImport/MaterialAssetImporter.h>

#include <Utils/Memory/ByteStream.h>
#include <Utils/Memory/ByteStreamUtils.h>

bool MaterialAssetImporter::Import(const Filepath& filePath)
{
	// @TODO Validate contents; currently only a missing/empty file is detected.
	ByteStream byteStream(filePath.GetRelativePath());
	if (!byteStream.ReadFromFile(filePath))
	{
		RZE_LOG_ARGS("Failed to read material asset [%s].", filePath.GetRelativePath().c_str());
		return false;
	}

	byteStream.PeekBytesAdvance(sizeof(size_t)); // bufSize

	mMaterialData.MaterialName = ByteStreamUtils::ReadString(byteStream);
	mMaterialData.Properties = ByteStreamUtils::ReadType<MaterialData::MaterialProperties>(byteStream);
	mMaterialData.TextureFlags = ByteStreamUtils::ReadType<U8>(byteStream);

	const size_t textureCount = *reinterpret_cast<size_t*>(byteStream.PeekBytesAdvance(sizeof(size_t)));
	for (size_t i = 0; i < textureCount; ++i)
	{
		mMaterialData.TexturePaths.push_back(ByteStreamUtils::ReadString(byteStream));
	}

	return true;
}
