#include <StdAfx.h>
#include <Game/MeshResource.h>

#include <Graphics/Material.h>
#include <Graphics/Texture2D.h>

#include <Graphics/Shader.h>

#include <Asset/AssetImport/MeshAssetImporter.h>

MeshResource::MeshResource()
{
}

MeshResource::~MeshResource()
{
}

bool MeshResource::Load(const FilePath& filePath)
{
	OPTICK_EVENT("MeshResource::Load");
	MeshAssetImporter meshImporter;
	meshImporter.Import(filePath);

	mMesh.Initialize(std::move(meshImporter.GetMeshGeometry()));
	return true;
}

void MeshResource::Release()
{
}
