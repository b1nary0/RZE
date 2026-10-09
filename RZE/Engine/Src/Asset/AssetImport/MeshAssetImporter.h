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
		MaterialAssetImporter::MaterialData Material;
	};

public:
	MeshAssetImporter() = default;
	virtual ~MeshAssetImporter() = default;

public:
	virtual bool Import(const Filepath& filePath) override;

	std::vector<ImportedSubmesh>& GetSubmeshes() { return m_submeshes; }

private:
	std::vector<ImportedSubmesh> m_submeshes;
};
