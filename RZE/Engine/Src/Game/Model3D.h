#pragma once

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
	StaticMesh mMesh;
};