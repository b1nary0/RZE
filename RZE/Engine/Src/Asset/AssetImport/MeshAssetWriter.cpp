#include <StdAfx.h>
#include <Asset/AssetImport/MeshAssetWriter.h>

#include <Asset/AssetPaths.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/Memory/ByteStream.h>
#include <Utils/Platform/File.h>

#include <algorithm>
#include <cstring>
#include <filesystem>

namespace
{
	constexpr uint16_t k_meshAssetVersion = 1;

	constexpr size_t k_placementSizeBytes = sizeof(U32) + sizeof(float) * 16;
}

void MeshAssetWriter::SetMeshData(std::vector<MeshData>&& meshData)
{
	m_meshes = std::move(meshData);
}

void MeshAssetWriter::SetPlacements(std::vector<MeshPlacement>&& placements)
{
	m_placements = std::move(placements);
}

void MeshAssetWriter::SetAssetName(const std::string& assetName)
{
	m_assetName = assetName;
}

void MeshAssetWriter::Write()
{
	AssertExpr(m_meshes.size() > 0);
	AssertExpr(!m_assetName.empty());

	if (m_placements.empty())
	{
		m_placements.reserve(m_meshes.size());
		for (size_t meshIndex = 0; meshIndex < m_meshes.size(); ++meshIndex)
		{
			MeshPlacement& placement = m_placements.emplace_back();
			placement.GeometryIndex = static_cast<U32>(meshIndex);
		}
	}

	{
		std::vector<bool> isPlaced(m_meshes.size(), false);
		for (const MeshPlacement& placement : m_placements)
		{
			AssertExpr(placement.GeometryIndex < m_meshes.size());
			isPlaced[placement.GeometryIndex] = true;
		}
		AssertExpr(std::find(isPlaced.begin(), isPlaced.end(), false) == isPlaced.end());
	}

	// #TODO really gross string manip lol
	const Filepath outputPath(AssetPaths::GetMeshFolder().GetRelativePath() + "/" + m_assetName + AssetPaths::k_meshExtension);
	if (!outputPath.Exists())
	{
		Filepath::CreateDir(outputPath.GetAbsoluteDirectoryPath());
	}

	File outputFile(outputPath);
	constexpr File::EFileOpenMode::Value k_openMode = static_cast<File::EFileOpenMode::Value>(File::EFileOpenMode::Write | File::EFileOpenMode::Binary);
	outputFile.Open(k_openMode);

	size_t bufSize = 0;
	for (auto& mesh : m_meshes)
	{
		bufSize += mesh.MeshName.size();
		bufSize += mesh.MaterialPath.size();
		bufSize += sizeof(MeshVertex) * mesh.VertexDataArray.size();
		bufSize += sizeof(U32) * mesh.IndexArray.size();
	}
	// magic numbers atm re: calculating data size headers per piece of data:
	// (file header + (nameSizeBytes + vertexDataSizeBytes + materialPathSizeBytes + indexDataSizeBytes) * meshCount
	const size_t meshCount = m_meshes.size();
	bufSize += sizeof(MeshAssetFileHeader) + ((sizeof(size_t) * 4) * meshCount);
	bufSize += k_placementSizeBytes * m_placements.size();

	// Written as raw bytes, so clear the padding after AssetVersion too; otherwise identical burns differ
	MeshAssetFileHeader header;
	std::memset(&header, 0, sizeof(MeshAssetFileHeader));
	header.AssetVersion = k_meshAssetVersion;
	header.BufSize = bufSize;
	header.MeshCount = meshCount;
	header.PlacementCount = m_placements.size();

	ByteStream byteStream(outputPath.GetRelativePath(), bufSize);

	byteStream.WriteBytes(&header, sizeof(MeshAssetFileHeader));

	for (auto& mesh : m_meshes)
	{
		// Have to piece-meal the copy here because the vectors are dynamically allocated. 
		const size_t nameSizeBytes = mesh.MeshName.size();
		byteStream.WriteBytes(&nameSizeBytes, sizeof(size_t));
		byteStream.WriteBytes(mesh.MeshName.data(), nameSizeBytes);

		const size_t materialPathSizeBytes = mesh.MaterialPath.size();
		byteStream.WriteBytes(&materialPathSizeBytes, sizeof(size_t));
		byteStream.WriteBytes(mesh.MaterialPath.data(), materialPathSizeBytes);

		const size_t vertexDataSizeBytes = sizeof(MeshVertex) * mesh.VertexDataArray.size();
		byteStream.WriteBytes(&vertexDataSizeBytes, sizeof(size_t));
		byteStream.WriteBytes(mesh.VertexDataArray.data(), vertexDataSizeBytes);

		const size_t indexDataSizeBytes = sizeof(U32) * mesh.IndexArray.size();
		byteStream.WriteBytes(&indexDataSizeBytes, sizeof(size_t));
		byteStream.WriteBytes(mesh.IndexArray.data(), indexDataSizeBytes);
	}

	for (const MeshPlacement& placement : m_placements)
	{
		byteStream.WriteBytes(&placement.GeometryIndex, sizeof(U32));
		byteStream.WriteBytes(placement.LocalTransform.GetValuePtr(), sizeof(float) * 16);
	}

	AssertExpr(byteStream.GetNumBytesWritten() == bufSize);

	const Byte* bytes = byteStream.GetBytes();
	for (size_t byteIndex = 0; byteIndex < byteStream.GetNumBytesWritten(); ++byteIndex)
	{
		outputFile << bytes[byteIndex];
	}

	outputFile.Close();

	RZE_LOG_ARGS("MeshAssetWriter : %s written.", outputPath.GetRelativePath().c_str());
}
