#pragma once

#include <string>
#include <memory.h>

#include <Utils/Interfaces/Resource.h>

struct aiMesh;
struct aiNode;
struct aiScene;

namespace Diotima
{
	class GFXMesh;
}

class Model3D : public IResource
{
public:
	Model3D();
	virtual ~Model3D();

public:
	virtual bool Load(const FilePath& filePath) override;
	virtual void Release() override;

public:
	inline std::vector<Diotima::GFXMesh*>& GetMeshList() { return mMeshList; }
	inline std::vector<ResourceHandle>& GetTextureHandles() { return mTextureHandles; }

private:
	void ProcessNode(const aiNode& node, const aiScene& scene);
	void ProcessMesh(const aiMesh& mesh, const aiScene& scene, Diotima::GFXMesh& outMesh);

	FilePath GetTextureFilePath(const std::string& fileName);

private:
	FilePath mFilePath;

	std::vector<Diotima::GFXMesh*> mMeshList;
	
	// Stored to keep references to the resource alive.
	std::vector<ResourceHandle> mTextureHandles;
};