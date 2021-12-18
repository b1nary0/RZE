#pragma once

#include <Asset/AssetImport/AssetImporter.h>

#include <Graphics/MeshGeometry.h>

#include <Utils/PrimitiveDefs.h>

class ByteStream;

class MeshAssetImporter : public AssetImporter
{
public:
	MeshAssetImporter() = default;
	virtual ~MeshAssetImporter() = default;

public:
	virtual bool Import(const FilePath& filePath) override;
	
	std::vector<MeshGeometry> GetMeshGeometry() { return mMeshGeometry; }

private:
	std::string ReadName(ByteStream& byteStream);
	std::vector<float> ReadVertices(ByteStream& byteStream);
	std::vector<U32> ReadIndices(ByteStream& byteStream);

private:
	std::vector<MeshGeometry> mMeshGeometry;
};