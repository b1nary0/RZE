#include <StdAfx.h>
#include <Asset/AssetImport/MeshAssetWriter.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/Memory/ByteStream.h>
#include <Utils/Platform/File.h>

#include <filesystem>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtx/matrix_decompose.hpp>

#include "Utils/Math/Matrix4x4.h"

namespace
{
	constexpr char k_meshAssetExtension[] = { ".meshasset" };
	constexpr uint16_t k_meshAssetVersion = 1;
}

void MeshAssetWriter::SetMeshData(std::vector<MeshData>&& meshData)
{
	m_meshes = std::move(meshData);
}

void MeshAssetWriter::SetAssetName(const std::string& assetName)
{
	m_assetName = assetName;
}

void MeshAssetWriter::Process()
{
	for (MeshData& meshData : m_meshes)
	{
		auto xExtremes = std::minmax_element(meshData.VertexDataArray.begin(), meshData.VertexDataArray.end(),
			[](const MeshVertex& lhs, const MeshVertex& rhs)
			{
				return lhs.Position.X() < rhs.Position.X();
			});

		auto yExtremes = std::minmax_element(meshData.VertexDataArray.begin(), meshData.VertexDataArray.end(),
			[](const MeshVertex& lhs, const MeshVertex& rhs)
			{
				return lhs.Position.Y() < rhs.Position.Y();
			});

		auto zExtremes = std::minmax_element(meshData.VertexDataArray.begin(), meshData.VertexDataArray.end(),
			[](const MeshVertex& lhs, const MeshVertex& rhs)
			{
				return lhs.Position.Z() < rhs.Position.Z();
			});

		float xMean = xExtremes.second->Position.X() / 2;
		float yMean = yExtremes.second->Position.Y() / 2;
		float zMean = zExtremes.second->Position.Z() / 2;


		m_centerPos = Vector3D(xMean, yMean, zMean);

		//for (MeshVertex& vertex : meshData.VertexDataArray)
		//{
		//	//Matrix4x4 matrix;
		//	//matrix.Translate(vertex.Position);
		//	//matrix = (matrix.Inverse() * Vector4D(m_centerPos, 1.0f)) * matrix;

		//	glm::mat4 matrix;
		//	glm::vec3 pos = vertex.Position.GetInternalVec();
		//	matrix = glm::translate(matrix, pos);

		//	glm::mat4 centerMatrix;
		//	glm::vec3 centerPos = m_centerPos.GetInternalVec();
		//	centerMatrix = glm::translate(centerMatrix, centerPos);

		//	glm::vec4 pos4 = glm::vec4(pos, 1.0f);
		//	matrix = matrix * centerMatrix * glm::inverse(matrix);
		//	
		//	glm::vec3 scale;
		//	glm::quat rotation;
		//	glm::vec3 translation;
		//	glm::vec3 skew;
		//	glm::vec4 perspective;
		//	glm::decompose(matrix, scale, rotation, translation, skew, perspective);

		//	vertex.Position = Vector3D(translation.x, translation.y, translation.z);
		//}
	}
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
	header.CenterPos = m_centerPos;

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

	RZE_LOG_ARGS("MeshAssetWriter : %s written.", outputPath.GetRelativePath().c_str());
}
