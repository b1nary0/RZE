#include <StdAfx.h>
#include <Game/StaticMeshResource.h>

#include <Asset/AssetImport/MeshAssetImporter.h>

#include <Utils/Conversions.h>

StaticMeshResource::StaticMeshResource()
{
}

StaticMeshResource::~StaticMeshResource()
{
}

bool StaticMeshResource::Load(const Filepath& filePath)
{
	OPTICK_EVENT("StaticMeshResource::Load");
	MeshAssetImporter meshImporter;
	meshImporter.Import(filePath);

	// TODO: Maybe send a StaticMesh& into MeshImporter::Import and do this boilerplate there
	m_mesh.Initialize(std::move(meshImporter.GetMeshGeometry()));

	std::string meshName = Conversions::StripAssetNameFromFilePath(filePath);
	m_mesh.SetName(meshName);

	return true;
}

void StaticMeshResource::Release()
{
}
