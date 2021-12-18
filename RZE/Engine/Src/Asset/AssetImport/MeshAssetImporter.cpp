#include <StdAfx.h>

#include <Asset/AssetImport/MeshAssetImporter.h>

#include <Utils/Memory/ByteStream.h>

namespace
{
	// #TODO
	// Read/Write this information to .meshasset
	constexpr int kAssetImportVersion = 1;
}

bool MeshAssetImporter::Import(const FilePath& filePath)
{
	ByteStream byteStream(filePath.GetRelativePath());
	byteStream.ReadFromFile(filePath);

	struct MeshAssetFileHeader
	{
		size_t BufSize;
		size_t MeshCount;
	};

	MeshAssetFileHeader* headerData = reinterpret_cast<MeshAssetFileHeader*>(byteStream.PeekBytesAdvance(sizeof(MeshAssetFileHeader)));
	size_t curPos = sizeof(MeshAssetFileHeader);
	for (int i = 0; i < headerData->MeshCount; ++i)
	{
		std::string meshName = ReadName(byteStream);
		std::vector<float> vertexData = ReadVertices(byteStream);
		std::vector<U32> indexData = ReadIndices(byteStream);

		std::vector<MeshVertex> meshVertexArray;

		MeshVertex* vertexDataArray = reinterpret_cast<MeshVertex*>(vertexData.data());
		meshVertexArray.insert(meshVertexArray.end(), &vertexDataArray[0], &vertexDataArray[vertexData.size() / (sizeof(MeshVertex) / 4)]);

		MeshGeometry geo;
		geo.SetName(meshName);
		geo.SetVertexData(meshVertexArray);
		geo.SetIndexData(indexData);
		geo.AllocateData();

		mMeshGeometry.push_back(geo);
	}

	return true;
}

std::string MeshAssetImporter::ReadName(ByteStream& byteStream)
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

std::vector<float> MeshAssetImporter::ReadVertices(ByteStream& byteStream)
{
	size_t vertexDataSizeBytes = *reinterpret_cast<size_t*>(byteStream.PeekBytesAdvance(sizeof(size_t)));

	std::vector<float> vertexDataVec;
	vertexDataVec.resize(vertexDataSizeBytes / sizeof(float));
	byteStream.ReadBytes((Byte*)vertexDataVec.data(), vertexDataSizeBytes);

	return vertexDataVec;
}

std::vector<U32> MeshAssetImporter::ReadIndices(ByteStream& byteStream)
{
	size_t indexDataSizeBytes = *reinterpret_cast<size_t*>(byteStream.PeekBytesAdvance(sizeof(size_t)));

	std::vector<U32> indexDataVec;
	indexDataVec.resize(indexDataSizeBytes / sizeof(U32));
	byteStream.ReadBytes((Byte*)indexDataVec.data(), indexDataSizeBytes);

	return indexDataVec;
}
