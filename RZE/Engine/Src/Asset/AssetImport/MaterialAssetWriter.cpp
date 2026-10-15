#include <StdAfx.h>
#include <Asset/AssetImport/MaterialAssetWriter.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/Memory/ByteStream.h>
#include <Utils/Platform/File.h>

namespace
{
	// 1: magic + version header; blend mode
	// 2: tint
	constexpr uint16_t k_materialAssetVersion = 2;

	void WriteString(ByteStream& byteStream, const std::string& string)
	{
		const size_t sizeBytes = string.size();
		byteStream.WriteBytes(&sizeBytes, sizeof(size_t));
		byteStream.WriteBytes(string.data(), sizeBytes);
	}
}

void MaterialAssetWriter::SetMaterialData(const MaterialAssetImporter::MaterialData& materialData)
{
	m_materialData = materialData;
}

void MaterialAssetWriter::SetOutputPath(const Filepath& outputPath)
{
	m_outputPath = outputPath;
}

void MaterialAssetWriter::Write()
{
	AssertExpr(m_outputPath.IsValid());

	if (!m_outputPath.Exists())
	{
		Filepath::CreateDir(m_outputPath.GetAbsoluteDirectoryPath());
	}

	using MaterialData = MaterialAssetImporter::MaterialData;

	size_t bufSize = sizeof(U32) + sizeof(uint16_t)
		+ sizeof(size_t) + m_materialData.MaterialName.size()
		+ sizeof(MaterialData::MaterialProperties) + sizeof(U8) + sizeof(MaterialData::EBlendMode)
		+ sizeof(float) * 3
		+ sizeof(size_t);
	for (const std::string& texturePath : m_materialData.TexturePaths)
	{
		bufSize += sizeof(size_t) + texturePath.size();
	}

	ByteStream byteStream(m_outputPath.GetRelativePath(), bufSize);

	byteStream.WriteBytes(&k_materialAssetMagic, sizeof(U32));
	byteStream.WriteBytes(&k_materialAssetVersion, sizeof(uint16_t));

	WriteString(byteStream, m_materialData.MaterialName);
	byteStream.WriteBytes(&m_materialData.Properties, sizeof(MaterialData::MaterialProperties));
	byteStream.WriteBytes(&m_materialData.TextureFlags, sizeof(U8));
	byteStream.WriteBytes(&m_materialData.BlendMode, sizeof(MaterialData::EBlendMode));
	const float tint[3] = { m_materialData.Tint.X(), m_materialData.Tint.Y(), m_materialData.Tint.Z() };
	byteStream.WriteBytes(tint, sizeof(tint));

	const size_t textureCount = m_materialData.TexturePaths.size();
	byteStream.WriteBytes(&textureCount, sizeof(size_t));
	for (const std::string& texturePath : m_materialData.TexturePaths)
	{
		WriteString(byteStream, texturePath);
	}

	AssertExpr(byteStream.GetNumBytesWritten() == bufSize);

	File outputFile(m_outputPath);
	outputFile.Open(static_cast<File::EFileOpenMode::Value>(File::EFileOpenMode::Write | File::EFileOpenMode::Binary));
	const Byte* const bytes = byteStream.GetBytes();
	for (size_t byteIndex = 0; byteIndex < byteStream.GetNumBytesWritten(); ++byteIndex)
	{
		outputFile << bytes[byteIndex];
	}
	outputFile.Close();

	RZE_LOG_ARGS("MaterialAssetWriter : %s written.", m_outputPath.GetRelativePath().c_str());
}
