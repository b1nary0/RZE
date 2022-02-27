#pragma once

#include <Graphics/StaticMesh.h>

#include <Utils/Interfaces/Resource.h>

struct aiMesh;
struct aiNode;
struct aiScene;

class MeshResource : public IResource
{
public:
	MeshResource();
	virtual ~MeshResource();

public:
	bool Load(const FilePath& filePath) override;
	void Release() override;

public:
	const StaticMesh& GetStaticMesh() const { return mMesh; }

private:
	StaticMesh mMesh;
};