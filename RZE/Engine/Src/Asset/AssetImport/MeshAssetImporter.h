#pragma once

#include <Asset/AssetImport/AssetImporter.h>
#include <Asset/AssetImport/MaterialAssetImporter.h>

#include <Graphics/MeshGeometry.h>

class ByteStream;

// Reads a .meshasset (and the material assets it references) into plain CPU data.
// Does no GPU or ResourceHandler work, so it is safe to run on a worker thread.
class MeshAssetImporter : public AssetImporter
{
public:
	struct ImportedSubmesh
	{
		std::string Name;
		std::vector<MeshVertex> Vertices;
		std::vector<U32> Indices;
		// Index into GetMaterials()
		U32 MaterialIndex = 0;
	};

	// A material asset the mesh uses, read once however many submeshes use it
	struct ImportedMaterial
	{
		// The asset's file name, unique within the mesh; "Default" for submeshes without one
		std::string Name;
		// Default (untextured) when the asset is missing
		MaterialAssetImporter::MaterialData Data;
	};

public:
	MeshAssetImporter() = default;
	virtual ~MeshAssetImporter() = default;

public:
	virtual bool Import(const Filepath& filePath) override;

	std::vector<ImportedSubmesh>& GetSubmeshes() { return m_submeshes; }
	// In the order submeshes first use them
	std::vector<ImportedMaterial>& GetMaterials() { return m_materials; }
	// Where each submesh is drawn; a submesh can appear more than once. Version 0 assets, which predate
	// placements, get one identity placement per submesh.
	std::vector<MeshPlacement>& GetPlacements() { return m_placements; }

private:
	std::vector<ImportedSubmesh> m_submeshes;
	std::vector<ImportedMaterial> m_materials;
	std::vector<MeshPlacement> m_placements;
};
