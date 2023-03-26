#pragma once

#include <Asset/AssetImport/AssetWriter.h>
#include <Graphics/MeshGeometry.h>

struct MeshData
{
	std::string MeshName;
	std::string MaterialPath;
	std::vector<MeshVertex> VertexDataArray;
	std::vector<U32> IndexArray;
};

struct MeshAssetFileHeader
{
	uint16_t AssetVersion;
	size_t BufSize;
	size_t MeshCount;
	Vector3D CenterPos;
};

class MeshAssetWriter : public AssetWriter
{
public:
	MeshAssetWriter() = default;

	void SetMeshData(std::vector<MeshData>&& meshData);
	void SetAssetName(const std::string& assetName);

	void Process() override;

	void Write() override;

private:
	Vector3D m_centerPos;
	std::vector<MeshData> m_meshes;
	std::string m_assetName;
};
