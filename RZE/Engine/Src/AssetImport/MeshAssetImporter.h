#pragma once

#include <AssetImport/AssetImporter.h>

#include <Graphics/MeshGeometry.h>

class MeshAssetImporter : public AssetImporter
{
public:
	MeshAssetImporter() = default;
	virtual ~MeshAssetImporter() = default;

public:
	virtual bool Import(const FilePath& filePath) override;
	
//private:
	std::vector<MeshGeometry> mMeshGeometry;
};