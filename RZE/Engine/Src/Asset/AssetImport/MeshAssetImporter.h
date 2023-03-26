#pragma once

#include <Asset/AssetImport/AssetImporter.h>

#include <Graphics/MeshGeometry.h>

class ByteStream;

class MeshAssetImporter : public AssetImporter
{
public:
	MeshAssetImporter() = default;
	virtual ~MeshAssetImporter() = default;

public:
	bool Import(const Filepath& filePath) override;
	
	std::vector<MeshGeometry> GetMeshGeometry() { return m_meshGeometry; }

	Vector3D m_centerPos;

private:
	std::vector<MeshGeometry> m_meshGeometry;
};