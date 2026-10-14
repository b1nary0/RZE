#pragma once

#include <SourceAssetBurner/Importers/SourceImporter.h>

#include <Asset/AssetImport/MeshAssetWriter.h>

#include <Utils/PrimitiveDefs.h>

#include <unordered_map>
#include <vector>

#include <Assimp/matrix4x4.h>

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
			TEXTUREFLAG_OPACITY = 1 << 3,
			TEXTUREFLAG_HEIGHT = 1 << 4,
			TEXTUREFLAG_OPACITY_IN_ALPHA = 1 << 5, // Opacity texture is the diffuse texture; cutout is in its alpha
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

	// Places the node's meshes with its transform relative to the model (parentTransform * the node's), then its children
	void ProcessNode(const aiNode& node, const aiMatrix4x4& parentTransform);
	void ProcessMesh(const aiMesh& mesh, const aiScene& scene, MeshData& outMeshData);

	// A mirrored placement turns its triangles inside out, so it draws a copy with the opposite winding
	U32 GetMirroredGeometryIndex(U32 geometryIndex);
	// Drops geometry no node places (the mesh asset needs every geometry placed)
	void RemoveUnplacedGeometry();

	bool WriteMeshAsset();
	bool WriteMaterialAsset(const std::string& relativePath, const MaterialData& materialData);
	bool WriteTextureAsset();
	
private:
	Filepath m_filepath;
	std::string m_assetName;

	// Indexed like aiScene::mMeshes, then any mirrored copies
	std::vector<MeshData> m_meshes;
	std::vector<MeshPlacement> m_placements;
	// Geometry index -> its mirrored copy's index
	std::unordered_map<U32, U32> m_mirroredGeometry;
	std::unordered_map<std::string, MaterialData> m_materialTable; // #TODO this is just a quick way to prevent duplicates. need a better solution in the future
	std::vector<Filepath> m_textures;
};
