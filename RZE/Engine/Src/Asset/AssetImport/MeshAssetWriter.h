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

// Version 1. Followed by MeshCount geometry blocks, then PlacementCount placements
// (U32 geometry index + 16 floats of local transform each)
struct MeshAssetFileHeader
{
	uint16_t AssetVersion;
	size_t BufSize;
	size_t MeshCount;
	size_t PlacementCount;
};

class MeshAssetWriter : public AssetWriter
{
public:
	MeshAssetWriter() = default;

	// Each entry is a unique piece of geometry, written once however many times it's placed
	void SetMeshData(std::vector<MeshData>&& meshData);
	// Optional. Without placements every geometry is placed once at the mesh's origin, which suits sources
	// with no node transforms (OBJ). Every geometry must be placed at least once.
	void SetPlacements(std::vector<MeshPlacement>&& placements);
	void SetAssetName(const std::string& assetName);

	void Write() override;

private:
	std::vector<MeshData> m_meshes;
	std::vector<MeshPlacement> m_placements;
	std::string m_assetName;
};
