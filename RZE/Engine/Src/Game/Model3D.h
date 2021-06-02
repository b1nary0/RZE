#pragma once

// #TODO
// Move Model3D.h/cpp out of Game/

#include <memory.h>
#include <string>

#include <Graphics/StaticMesh.h>

#include <Utils/Interfaces/Resource.h>

struct aiMesh;
struct aiNode;
struct aiScene;

class Model3D : public IResource
{
public:
	Model3D();
	virtual ~Model3D();

public:
	virtual bool Load(const FilePath& filePath) override;
	virtual void Release() override;

public:
	inline const StaticMesh& GetStaticMesh() const { return mMesh; }

private:
	void ProcessNode(const aiNode& node, const aiScene& scene, std::vector<MeshGeometry>& outMeshGeometry);
	void ProcessMesh(const aiMesh& mesh, const aiScene& scene, MeshGeometry& outMesh);

	FilePath GetTextureFilePath(const std::string& fileName);

private:
	FilePath mFilePath;

	StaticMesh mMesh;
};