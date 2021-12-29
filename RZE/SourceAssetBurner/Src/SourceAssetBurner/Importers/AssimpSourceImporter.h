#pragma once

#include <SourceAssetBurner/Importers/SourceImporter.h>

#include <unordered_map>
#include <vector>

#include <Utils/Math/Vector2D.h>
#include <Utils/Math/Vector3D.h>

#include <Utils/PrimitiveDefs.h>

// #TODO should hide these behind some interface so we can swap out RapidJSON when we want to
#include <RapidJSON/document.h>
#include <RapidJSON/prettywriter.h>
#include <RapidJSON/stringbuffer.h>

struct aiMesh;
struct aiNode;
struct aiScene;

// #TODO
// Eventually move the writing code out and the importers just store the data from the relevant
// import library (assimp etc) but the data is transformed to the same RZE concepts and written the
// same way via "globally" accessible functions (like a writing class or something? i dunno need to think
// on it more)

class AssimpSourceImporter : public SourceImporter
{
public:
	AssimpSourceImporter();
	virtual ~AssimpSourceImporter() = default;

	virtual bool Import(const FilePath& filePath) override;

private:
	struct MeshVertex
	{
		Vector3D Position;
		Vector3D Normal;
		Vector3D Tangent;
		Vector2D UVData;
	};

	struct MeshData
	{
		std::string MeshName;
		std::string MaterialPath;
		std::vector<MeshVertex> VertexDataArray;
		std::vector<U32> IndexArray;
	};

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
	FilePath mFilePath;
	std::string mAssetName;

	std::vector<MeshData> mMeshes;
	std::unordered_map<std::string, MaterialData> mMaterialTable; // #TODO this is just a quick way to prevent duplicates. need a better solution in the future
	std::vector<FilePath> mTextures;
};
