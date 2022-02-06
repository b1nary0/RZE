#pragma once

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
	bool Load(const FilePath& filePath) override;
	void Release() override;

public:
	const StaticMesh& GetStaticMesh() const { return mMesh; }

private:
	StaticMesh mMesh;
};