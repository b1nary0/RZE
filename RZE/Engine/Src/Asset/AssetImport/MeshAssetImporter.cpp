#include <StdAfx.h>

#include <Asset/AssetImport/MeshAssetImporter.h>
#include <Asset/AssetImport/MeshAssetWriter.h>
#include <Asset/AssetImport/MaterialAssetImporter.h>

#include <Utils/Memory/ByteStream.h>
#include <Utils/Memory/ByteStreamUtils.h>

#include <cstddef>
#include <filesystem>
#include <unordered_map>

namespace
{
	constexpr uint16_t k_meshImporterVersion = 1;
	// Assets burned before placements existed; read as one identity placement per submesh
	constexpr uint16_t k_meshImporterVersionNoPlacements = 0;

	// The version 0 header. Version 1 (MeshAssetFileHeader) starts with the same fields and adds PlacementCount.
	struct MeshAssetFileHeaderV0
	{
		uint16_t AssetVersion;
		size_t BufSize;
		size_t MeshCount;
	};
	static_assert(offsetof(MeshAssetFileHeader, PlacementCount) == sizeof(MeshAssetFileHeaderV0));
}

bool MeshAssetImporter::Import(const Filepath& filePath)
{
	m_submeshes.clear();
	m_materials.clear();
	m_placements.clear();

	ByteStream byteStream(filePath.GetRelativePath());
	if (!byteStream.ReadFromFile(filePath) || byteStream.GetLength() < sizeof(MeshAssetFileHeaderV0))
	{
		RZE_LOG_ERROR_ARGS("Failed to read mesh asset [%s].", filePath.GetRelativePath().c_str());
		return false;
	}

	const MeshAssetFileHeaderV0 headerData = ByteStreamUtils::ReadType<MeshAssetFileHeaderV0>(byteStream);
	const bool hasPlacements = headerData.AssetVersion == k_meshImporterVersion;
	if (!hasPlacements && headerData.AssetVersion != k_meshImporterVersionNoPlacements)
	{
		RZE_LOG_ERROR_ARGS("Mesh asset [%s] has version %u, expected %u.", filePath.GetRelativePath().c_str(), static_cast<U32>(headerData.AssetVersion), static_cast<U32>(k_meshImporterVersion));
		return false;
	}

	if (hasPlacements && byteStream.GetLength() < sizeof(MeshAssetFileHeader))
	{
		RZE_LOG_ERROR_ARGS("Failed to read mesh asset [%s].", filePath.GetRelativePath().c_str());
		return false;
	}
	const size_t placementCount = hasPlacements ? ByteStreamUtils::ReadType<size_t>(byteStream) : headerData.MeshCount;

	// Material asset path -> index into m_materials
	std::unordered_map<std::string, U32> materialIndices;

	m_submeshes.reserve(headerData.MeshCount);
	for (size_t meshIndex = 0; meshIndex < headerData.MeshCount; ++meshIndex)
	{
		ImportedSubmesh& submesh = m_submeshes.emplace_back();

		submesh.Name = ByteStreamUtils::ReadString(byteStream);
		std::string materialPath = ByteStreamUtils::ReadString(byteStream);
		std::vector<float> vertexData = ByteStreamUtils::ReadArray<float>(byteStream);
		submesh.Indices = ByteStreamUtils::ReadArray<U32>(byteStream);

		const MeshVertex* const vertexDataArray = reinterpret_cast<const MeshVertex*>(vertexData.data());
		const size_t vertexCount = vertexData.size() / (sizeof(MeshVertex) / sizeof(float));
		submesh.Vertices.assign(vertexDataArray, vertexDataArray + vertexCount);

		auto knownMaterial = materialIndices.find(materialPath);
		if (knownMaterial != materialIndices.end())
		{
			submesh.MaterialIndex = knownMaterial->second;
			continue;
		}

		submesh.MaterialIndex = static_cast<U32>(m_materials.size());
		materialIndices.emplace(materialPath, submesh.MaterialIndex);

		ImportedMaterial& material = m_materials.emplace_back();
		material.Name = materialPath.empty() ? std::string("Default") : std::filesystem::path(materialPath).stem().string();
		material.Path = materialPath;

		// Missing materials are tolerated: the submesh gets a default (untextured, grey) material.
		MaterialAssetImporter materialImporter;
		if (!materialPath.empty() && materialImporter.Import(Filepath(materialPath)))
		{
			material.Data = materialImporter.GetMaterialData();
		}
		else
		{
			material.Data.Tint = MaterialAssetImporter::GetLegacyTint(MaterialAssetImporter::MaterialData::TEXTUREFLAG_NONE);
		}
	}

	m_placements.reserve(placementCount);
	for (size_t placementIndex = 0; placementIndex < placementCount; ++placementIndex)
	{
		MeshPlacement& placement = m_placements.emplace_back();
		if (!hasPlacements)
		{
			placement.GeometryIndex = static_cast<U32>(placementIndex);
			continue;
		}

		placement.GeometryIndex = ByteStreamUtils::ReadType<U32>(byteStream);
		float localTransform[16];
		byteStream.ReadBytes(reinterpret_cast<Byte*>(localTransform), sizeof(localTransform));
		placement.LocalTransform = Matrix4x4::FromValuePtr(localTransform);

		if (placement.GeometryIndex >= m_submeshes.size())
		{
			RZE_LOG_ERROR_ARGS("Mesh asset [%s] places submesh %u, but only has %u.", filePath.GetRelativePath().c_str(), placement.GeometryIndex, static_cast<U32>(m_submeshes.size()));
			m_submeshes.clear();
			m_materials.clear();
			m_placements.clear();
			return false;
		}
	}

	return true;
}
