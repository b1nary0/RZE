#include <StdAfx.h>
#include <Game/StaticMeshResource.h>

#include <Graphics/Material.h>
#include <Graphics/Texture2D.h>

#include <Graphics/Shader.h>

#include <Asset/AssetImport/MeshAssetImporter.h>

StaticMeshResource::StaticMeshResource()
{
}

StaticMeshResource::~StaticMeshResource()
{
}

bool StaticMeshResource::Load(const FilePath& filePath)
{
	OPTICK_EVENT("StaticMeshResource::Load");
	MeshAssetImporter meshImporter;
	meshImporter.Import(filePath);

	mMesh.Initialize(std::move(meshImporter.GetMeshGeometry()));
	return true;
}

void StaticMeshResource::Release()
{
}
