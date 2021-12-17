#include <StdAfx.h>

#include <Asset/AssetImport/MeshAssetImporter.h>

#include <EngineCore/Platform/Memory/ByteStream.h>

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

	Byte* readBytes = byteStream.GetByteStream();

	struct MeshAssetFileHeader
	{
		size_t BufSize;
		size_t MeshCount;
	};

	MeshAssetFileHeader* headerData = reinterpret_cast<MeshAssetFileHeader*>(readBytes);
	size_t curPos = sizeof(MeshAssetFileHeader);
	for (int i = 0; i < headerData->MeshCount; ++i)
	{
		std::string meshName = ReadName(readBytes, curPos);
		std::vector<float> vertexData = ReadVertices(readBytes, curPos);
		std::vector<U32> indexData = ReadIndices(readBytes, curPos);

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

// #TODO
// Should be able to just copy directly from readBytes instead of 
// creating a buffer on the heap each time for the functions below.
std::string MeshAssetImporter::ReadName(Byte* readBytes, size_t& curPos)
{
	size_t nameSizeBytes = *reinterpret_cast<size_t*>(&readBytes[curPos]);
	curPos += sizeof(size_t);

	unsigned char* name = new unsigned char[nameSizeBytes + 1];
	memcpy(name, &readBytes[curPos], nameSizeBytes);
	name[nameSizeBytes] = '\0';
	std::string string((char*)name);
	curPos += nameSizeBytes;

	delete[] name;
	name = nullptr;
	return std::move(string);
}

std::vector<float> MeshAssetImporter::ReadVertices(Byte* readBytes, size_t& curPos)
{
	size_t vertexDataSizeBytes = *reinterpret_cast<size_t*>(&readBytes[curPos]);
	curPos += sizeof(size_t);

	std::vector<float> vertexDataVec;
	vertexDataVec.resize(vertexDataSizeBytes / sizeof(float));
	memcpy(vertexDataVec.data(), &readBytes[curPos], vertexDataSizeBytes);
	curPos += vertexDataSizeBytes;

	return vertexDataVec;
}

std::vector<U32> MeshAssetImporter::ReadIndices(Byte* readBytes, size_t& curPos)
{
	size_t indexDataSizeBytes = *reinterpret_cast<size_t*>(&readBytes[curPos]);
	curPos += sizeof(size_t);

	std::vector<U32> indexDataVec;
	indexDataVec.resize(indexDataSizeBytes / sizeof(U32));
	memcpy(indexDataVec.data(), &readBytes[curPos], indexDataSizeBytes);
	curPos += indexDataSizeBytes;

	return indexDataVec;
}
