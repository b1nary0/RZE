#include <StdAfx.h>
#include <Game/Model3D.h>

#include <Assimp/Importer.hpp>
#include <Assimp/postprocess.h>
#include <Assimp/scene.h>

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
	mFilePath = filePath;

	MeshAssetImporter meshImporter;
	meshImporter.Import(filePath);

	mMesh.Initialize(std::move(meshImporter.GetMeshGeometry()));
	return true;
}

void Model3D::Release()
{
}
