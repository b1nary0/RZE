#include <StdAfx.h>
#include <Asset/AssetImport/MaterialAssetWriter.h>

#include <Utils/DebugUtils/Debug.h>

#include <Utils/Memory/ByteStream.h>

namespace
{
	constexpr char k_materialAssetExtension[] = { ".materialasset" };
	constexpr uint16_t k_materialAssetVersion = 0;
}

void MaterialAssetWriter::SetMaterialData(const MaterialData& materialData)
{
	m_materialData = materialData;
}

void MaterialAssetWriter::SetAssetName(const std::string& assetName)
{
	m_assetName = assetName;
}

void MaterialAssetWriter::Write()
{
	// #TODO really gross string manip lol
	std::string assetFilename = m_assetName + k_materialAssetExtension;
	std::string assetFilepathRelative = "ProjectData/Material/";
	assetFilepathRelative = assetFilepathRelative + assetFilename;

	Filepath outputPath(assetFilepathRelative);
	if (!outputPath.Exists())
	{
		Filepath::CreateDir(outputPath.GetAbsoluteDirectoryPath());
	}

	File outputFile(outputPath);
	constexpr File::EFileOpenMode::Value k_openMode = static_cast<File::EFileOpenMode::Value>(File::EFileOpenMode::Write | File::EFileOpenMode::Binary);
	outputFile.Open(k_openMode);

	size_t bufSize = 0;
	bufSize += sizeof(MaterialAssetFileHeader);
	bufSize += sizeof(size_t); // material name size
	bufSize += m_materialData.MaterialName.size();
	bufSize += sizeof(MaterialData::MaterialProperties);
	bufSize += sizeof(U8);
	bufSize += sizeof(size_t); // texture count

	for (const std::string& texturePath : m_materialData.TexturePaths)
	{
		// count the bytes of the texture string size in sizeof(size_t) here
		bufSize += sizeof(size_t) + texturePath.size();
	}

	MaterialAssetFileHeader header;
	header.AssetVersion = k_materialAssetVersion;
	header.BufSize = bufSize;

	ByteStream byteStream(outputPath.GetRelativePath(), bufSize);

	byteStream.WriteBytes(&header, sizeof(MaterialAssetFileHeader));

	const size_t nameSizeBytes = m_materialData.MaterialName.size();
	byteStream.WriteBytes(&nameSizeBytes, sizeof(size_t));
	byteStream.WriteBytes(m_materialData.MaterialName.data(), nameSizeBytes);

	byteStream.WriteBytes(&m_materialData.Properties, sizeof(MaterialData::MaterialProperties));
	byteStream.WriteBytes(&m_materialData.TextureFlags, sizeof(U8));

	const size_t kTextureCount = m_materialData.TexturePaths.size();
	byteStream.WriteBytes(&kTextureCount, sizeof(size_t));
	for (int textureIdx = 0; textureIdx < kTextureCount; ++textureIdx)
	{
		const std::string& texturePath = m_materialData.TexturePaths[textureIdx];

		const size_t pathSizeBytes = texturePath.size();
		byteStream.WriteBytes(&pathSizeBytes, sizeof(size_t));
		byteStream.WriteBytes(texturePath.data(), pathSizeBytes);
	}

	AssertExpr(byteStream.GetNumBytesWritten() == bufSize);

	Byte* bytes = byteStream.GetBytes();
	for (size_t byteIndex = 0; byteIndex < byteStream.GetNumBytesWritten(); ++byteIndex)
	{
		outputFile << bytes[byteIndex];
	}

	outputFile.Close();

	RZE_LOG_ARGS("MaterialAssetWriter : %s written.", outputPath.GetRelativePath().c_str());
}
