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
	virtual bool Import(const Filepath& filePath) override;
	
	std::vector<MeshGeometry> GetMeshGeometry() { return mMeshGeometry; }

private:
	std::vector<MeshGeometry> mMeshGeometry;
};