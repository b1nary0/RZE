#pragma once

#include <AssetImport/AssetImporter.h>

class MeshAssetImporter : public AssetImporter
{
public:
	MeshAssetImporter() = default;
	virtual ~MeshAssetImporter() = default;

public:
	virtual bool Import(const FilePath& filePath) override;
	
private:
	struct MeshData
	{
		std::vector<float> VertexData;
	};
	std::vector<MeshData> test;
};