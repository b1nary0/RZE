#pragma once

#include <string>

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
	virtual bool Load(const std::string& filePath) override;
	virtual void Release() override;

public:
	inline std::vector<Diotima::GFXMesh*>& GetMeshList() { return mMeshList; }
	inline std::vector<ResourceHandle>& GetTextureHandles() { return mTextureHandles; }

private:
	void ProcessNode(const aiNode& node, const aiScene& scene);
	void ProcessMesh(const aiMesh& mesh, const aiScene& scene, Diotima::GFXMesh& outMesh);
	std::vector<Diotima::GFXMesh*> mMeshList;

	std::vector<ResourceHandle> mTextureHandles;
};