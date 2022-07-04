#include <StdAfx.h>
#include <Asset/AssetImport/MeshAssetWriter.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/Memory/ByteStream.h>
#include <Utils/Platform/File.h>

#include <filesystem>

namespace
{
	constexpr char k_meshAssetExtension[] = { ".meshasset" };
	constexpr uint16_t k_meshAssetVersion = 0;
}

void MeshAssetWriter::SetMeshData(std::vector<MeshData>&& meshData)
{
	m_meshes = std::move(meshData);
}

void MeshAssetWriter::SetAssetName(const std::string& assetName)
{
	m_assetName = assetName;
}

void MeshAssetWriter::Write()
{
	AssertExpr(m_meshes.size() > 0);
	AssertExpr(!m_assetName.empty());

	// #TODO really gross string manip lol
	std::string assetFilename = m_assetName + k_meshAssetExtension;
	std::string assetFilepathRelative = "ProjectData/Mesh/";
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
	for (auto& mesh : m_meshes)
	{
		bufSize += mesh.MeshName.size();
		bufSize += mesh.MaterialPath.size();
		bufSize += sizeof(MeshVertex) * mesh.VertexDataArray.size();
		bufSize += sizeof(U32) * mesh.IndexArray.size();
	}
	// magic numbers atm re: calculating data size headers per piece of data:
	// (file header + (nameSizeBytes + vertexDataSizeBytes + materialPathSizeBytes + indexDataSizeBytes) * meshCount
	bufSize += sizeof(MeshAssetFileHeader) + ((sizeof(size_t) * 4) * m_meshes.size());
	const size_t meshCount = m_meshes.size();

	MeshAssetFileHeader header;
	header.AssetVersion = k_meshAssetVersion;
	header.BufSize = bufSize;
	header.MeshCount = meshCount;

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

	AssertExpr(byteStream.GetNumBytesWritten() == bufSize);

	Byte* bytes = byteStream.GetBytes();
	for (size_t byteIndex = 0; byteIndex < byteStream.GetNumBytesWritten(); ++byteIndex)
	{
		outputFile << bytes[byteIndex];
	}

	outputFile.Close();

	LOG_CONSOLE_ARGS("MeshAssetWriter : %s written.", outputPath.GetRelativePath().c_str());
}
