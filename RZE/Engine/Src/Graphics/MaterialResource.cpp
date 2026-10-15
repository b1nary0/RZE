#include <StdAfx.h>
#include <Graphics/MaterialResource.h>

#include <EngineCore/Resources/AsyncResourceBatch.h>

#include <Graphics/Material.h>
#include <Graphics/Texture2D.h>

MaterialResource::MaterialResource()
{
}

MaterialResource::~MaterialResource()
{
}

bool MaterialResource::Load(const Filepath& filePath)
{
	if (!LoadCPU(filePath))
	{
		return false;
	}

	while (!FinalizeStep())
	{
	}

	return true;
}

bool MaterialResource::LoadCPU(const Filepath& filePath)
{
	MaterialAssetImporter importer;
	if (!importer.Import(filePath))
	{
		return false;
	}

	m_pendingData = importer.GetMaterialData();
	m_isFinalizePending = true;
	return true;
}

void MaterialResource::RequestDependencies(AsyncResourceBatch& batch) const
{
	for (const std::string& texturePath : m_pendingData.TexturePaths)
	{
		if (!texturePath.empty())
		{
			batch.Request<Texture2D>(Filepath(texturePath));
		}
	}
}

bool MaterialResource::FinalizeStep()
{
	if (!m_isFinalizePending)
	{
		return true;
	}

	// Its textures are cache hits when loaded through AsyncResourceBatch, which finalizes them first
	m_material = MaterialInstance::Create(m_pendingData);

	m_pendingData = MaterialAssetImporter::MaterialData();
	m_isFinalizePending = false;
	return true;
}

void MaterialResource::Release()
{
	// Whatever draws with the material keeps it alive until it's done
	m_material.reset();
}
