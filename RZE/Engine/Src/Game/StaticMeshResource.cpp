#include <StdAfx.h>
#include <Game/StaticMeshResource.h>

#include <Asset/AssetImport/MeshAssetImporter.h>

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

	m_mesh.Initialize(std::move(meshImporter.GetMeshGeometry()));
	m_mesh.m_centerPos = meshImporter.m_centerPos;

	return true;
}

void StaticMeshResource::Release()
{
}
