#pragma once

#include <SourceAssetBurner/Importers/SourceImporter.h>

#include <Asset/AssetImport/MeshAssetWriter.h>
#include <Asset/AssetImport/MaterialAssetWriter.h>

#include <Utils/PrimitiveDefs.h>

#include <unordered_map>
#include <vector>

struct aiMesh;
struct aiNode;
struct aiScene;

class AssimpSourceImporter final : public SourceImporter
{
public:
	AssimpSourceImporter();
	~AssimpSourceImporter() override = default;

	virtual bool Import(const Filepath& filePath) override;

private:
	void ProcessNode(const aiNode& node, const aiScene& scene);
	void ProcessMesh(const aiMesh& mesh, const aiScene& scene, MeshData& outMeshData);

	bool WriteMeshAsset();
	bool WriteMaterialAsset(const MaterialData& materialData);
	bool WriteTextureAsset();
	
private:
	Filepath m_filepath;
	std::string m_assetName;

	std::vector<MeshData> m_meshes;
	std::unordered_map<std::string, MaterialData> m_materialTable; // #TODO this is just a quick way to prevent duplicates. need a better solution in the future
	std::vector<Filepath> m_textures;
};
