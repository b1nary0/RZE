#include <StdAfx.h>
#include <Game/Model3D.h>

#include <Graphics/Material.h>
#include <Graphics/Texture2D.h>

#include <Graphics/Shader.h>

#include <Asset/AssetImport/MeshAssetImporter.h>

Model3D::Model3D()
{
}

Model3D::~Model3D()
{
}

bool Model3D::Load(const FilePath& filePath)
{
	OPTICK_EVENT("Model3D::Load");
	MeshAssetImporter meshImporter;
	meshImporter.Import(filePath);

	mMesh.Initialize(std::move(meshImporter.GetMeshGeometry()));
	return true;
}

void Model3D::Release()
{
}
