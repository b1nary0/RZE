#include <StdAfx.h>

#include <Asset/AssetImport/MeshAssetImporter.h>
#include <Asset/AssetImport/MeshAssetWriter.h>
#include <Asset/AssetImport/MaterialAssetImporter.h>

#include <Utils/Memory/ByteStream.h>
#include <Utils/Memory/ByteStreamUtils.h>

namespace
{
	constexpr uint16_t k_meshImporterVersion = 0;
}

bool MeshAssetImporter::Import(const Filepath& filePath)
{
	m_submeshes.clear();

	ByteStream byteStream(filePath.GetRelativePath());
	if (!byteStream.ReadFromFile(filePath) || byteStream.GetLength() < sizeof(MeshAssetFileHeader))
	{
		RZE_LOG_ARGS("Failed to read mesh asset [%s].", filePath.GetRelativePath().c_str());
		return false;
	}

	MeshAssetFileHeader* headerData = reinterpret_cast<MeshAssetFileHeader*>(byteStream.PeekBytesAdvance(sizeof(MeshAssetFileHeader)));
	if (headerData->AssetVersion != k_meshImporterVersion)
	{
		RZE_LOG_ARGS("Mesh asset [%s] has version %u, expected %u.", filePath.GetRelativePath().c_str(), static_cast<U32>(headerData->AssetVersion), static_cast<U32>(k_meshImporterVersion));
		return false;
	}

	m_submeshes.reserve(headerData->MeshCount);
	for (size_t meshIndex = 0; meshIndex < headerData->MeshCount; ++meshIndex)
	{
		ImportedSubmesh& submesh = m_submeshes.emplace_back();

		submesh.Name = ByteStreamUtils::ReadString(byteStream);
		std::string materialPath = ByteStreamUtils::ReadString(byteStream);
		std::vector<float> vertexData = ByteStreamUtils::ReadArray<float>(byteStream);
		submesh.Indices = ByteStreamUtils::ReadArray<U32>(byteStream);

		const MeshVertex* const vertexDataArray = reinterpret_cast<const MeshVertex*>(vertexData.data());
		const size_t vertexCount = vertexData.size() / (sizeof(MeshVertex) / sizeof(float));
		submesh.Vertices.assign(vertexDataArray, vertexDataArray + vertexCount);

		// Missing materials are tolerated: the submesh gets a default (untextured) material.
		MaterialAssetImporter materialImporter;
		if (materialImporter.Import(Filepath(materialPath)))
		{
			submesh.Material = materialImporter.GetMaterialData();
		}
	}

	return true;
}
