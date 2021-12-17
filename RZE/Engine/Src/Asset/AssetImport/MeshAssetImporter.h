#pragma once

#include <Asset/AssetImport/AssetImporter.h>

#include <Graphics/MeshGeometry.h>

#include <Utils/PrimitiveDefs.h>

class MeshAssetImporter : public AssetImporter
{
public:
	MeshAssetImporter() = default;
	virtual ~MeshAssetImporter() = default;

public:
	virtual bool Import(const FilePath& filePath) override;
	
	std::vector<MeshGeometry> GetMeshGeometry() { return mMeshGeometry; }

private:
	std::string ReadName(Byte* readBytes, size_t& curPos);
	std::vector<float> ReadVertices(Byte* readBytes, size_t& curPos);
	std::vector<U32> ReadIndices(Byte* readBytes, size_t& curPos);

private:
	std::vector<MeshGeometry> mMeshGeometry;
};