#pragma once

#include <SourceAssetBurner/Importers/SourceImporter.h>

#include <Asset/AssetImport/MeshAssetWriter.h>

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

	virtual bool Import(const FilePath& filePath) override;

private:
	// #TODO
	// standardize these data structures so they dont all need to be maintained...
	struct MaterialData
	{
		enum ETextureFlags
		{
			TEXTUREFLAG_NONE = 0,
			TEXTUREFLAG_ALBEDO = 1 << 0,
			TEXTUREFLAG_SPECULAR = 1 << 1,
			TEXTUREFLAG_NORMAL = 1 << 2,
			TEXTUREFLAG_ALL = TEXTUREFLAG_ALBEDO | TEXTUREFLAG_SPECULAR |  TEXTUREFLAG_NORMAL
		};

		std::string MaterialName;
		struct MaterialProperties
		{
			float Shininess = 0.0f;;
			float Opacity = 1.0f;
		} Properties;
		// #TODO These flags inform which texture types are used by this material
		// probably not the best implementation of this but i think the asset infrastructure
		// needs to mature.
		U8 TextureFlags = TEXTUREFLAG_NONE;
		std::vector<std::string> TexturePaths;
	};

	void ProcessNode(const aiNode& node, const aiScene& scene);
	void ProcessMesh(const aiMesh& mesh, const aiScene& scene, MeshData& outMeshData);

	bool WriteMeshAsset();
	bool WriteMaterialAsset(const std::string& relativePath, const MaterialData& materialData);
	bool WriteTextureAsset();
	
private:
	FilePath m_filepath;
	std::string m_assetName;

	std::vector<MeshData> m_meshes;
	std::unordered_map<std::string, MaterialData> m_materialTable; // #TODO this is just a quick way to prevent duplicates. need a better solution in the future
	std::vector<FilePath> m_textures;
};
