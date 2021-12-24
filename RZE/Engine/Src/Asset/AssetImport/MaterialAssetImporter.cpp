#include <StdAfx.h>
#include <Asset/AssetImport/MaterialAssetImporter.h>

#include <Utils/Memory/ByteStream.h>
#include <Utils/Memory/ByteStreamUtils.h>

bool MaterialAssetImporter::Import(const FilePath& filePath)
{
	ByteStream byteStream(filePath.GetRelativePath());
	byteStream.ReadFromFile(filePath);

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
