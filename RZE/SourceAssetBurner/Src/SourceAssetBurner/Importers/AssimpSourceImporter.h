#pragma once

#include <SourceAssetBurner/Importers/SourceImporter.h>

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
		std::vector<MeshVertex> VertexDataArray;
		std::vector<U32> IndexArray;
	};

	struct MaterialData
	{
		std::string MaterialName;
	};

	void ProcessNode(const aiNode& node, const aiScene& scene);
	void ProcessMesh(const aiMesh& mesh, const aiScene& scene, MeshData& outMeshData);

	// #TODO this is first pass stuff, ideally some reflection system facilitates this stuff
	// the manual json format is a maintenance nightmare but quick to implement. Should get to
	// this before it gets out of hand. Just wrote myself into more non-rendering work lol
	bool WriteMeshFile();
	void WriteSingleMesh(const MeshData& meshData);
	
private:
	std::string mAssetName;

	std::vector<MeshData> mMeshes;
	// #TODO will need to consider duplicate material references across meshes for this implementation.
	std::vector<MaterialData> mMaterials;

	rapidjson::StringBuffer mStringBuffer;
	rapidjson::Writer<rapidjson::StringBuffer> mWriter;
};
