#pragma once

#include <Graphics/StaticMesh.h>

#include <Utils/Interfaces/Resource.h>

struct aiMesh;
struct aiNode;
struct aiScene;

class StaticMeshResource : public IResource
{
public:
	StaticMeshResource();
	virtual ~StaticMeshResource();

public:
	bool Load(const Filepath& filePath) override;
	void Release() override;

public:
	const StaticMesh& GetStaticMesh() const { return mMesh; }

private:
	StaticMesh mMesh;
};