#pragma once

#include <SourceAssetBurner/Importers/SourceImporter.h>

#include <vector>

#include <Utils/Math/Vector2D.h>
#include <Utils/Math/Vector3D.h>

#include <Utils/PrimitiveDefs.h>

struct aiMesh;
struct aiNode;
struct aiScene;

class AssimpSourceImporter : public SourceImporter
{
public:
	AssimpSourceImporter() = default;
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
		std::vector<MeshVertex> VertexDataArray;
		std::vector<U32> IndexArray;
	};

	struct MaterialData
	{
		std::string MaterialName;
	};

	void ProcessNode(const aiNode& node, const aiScene& scene);
	void ProcessMesh(const aiMesh& mesh, const aiScene& scene, MeshData& outMeshData);

private:
	std::vector<MeshData> mMeshes;
	// #TODO will need to consider duplicate material references across meshes here.
	std::vector<MaterialData> mMaterials;
};
