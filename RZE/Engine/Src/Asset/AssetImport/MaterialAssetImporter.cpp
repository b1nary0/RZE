#include <StdAfx.h>
#include <Asset/AssetImport/MaterialAssetImporter.h>

#include <Utils/Memory/ByteStream.h>

bool MaterialAssetImporter::Import(const FilePath& filePath)
{
	ByteStream byteStream(filePath.GetRelativePath());
	byteStream.ReadFromFile(filePath);

	byteStream.PeekBytesAdvance(sizeof(size_t)); // bufSize

	mMaterialData.MaterialName = ReadName(byteStream);
	mMaterialData.Properties = ReadMaterialProperties(byteStream);
	mMaterialData.TextureFlags = ReadTextureFlags(byteStream);

	return true;
}

std::string MaterialAssetImporter::ReadName(ByteStream& byteStream)
{
	size_t nameSizeBytes = *reinterpret_cast<size_t*>(byteStream.PeekBytesAdvance(sizeof(size_t)));

	unsigned char* name = new unsigned char[nameSizeBytes + 1];
	byteStream.ReadBytes(name, nameSizeBytes);
	name[nameSizeBytes] = '\0';
	std::string string((char*)name);

	delete[] name;
	name = nullptr;
	return std::move(string);
}

MaterialAssetImporter::MaterialData::MaterialProperties 
MaterialAssetImporter::ReadMaterialProperties(ByteStream& byteStream)
{
	MaterialData::MaterialProperties* properties = 
		reinterpret_cast<MaterialData::MaterialProperties*>(byteStream.PeekBytesAdvance(sizeof(MaterialData::MaterialProperties)));
	return *properties;
}

U8 MaterialAssetImporter::ReadTextureFlags(ByteStream& byteStream)
{
	return *((U8*)byteStream.PeekBytesAdvance(sizeof(U8)));
}
