#include <StdAfx.h>
#include <Game/StaticMeshResource.h>

#include <EngineCore/Resources/AsyncResourceBatch.h>

#include <Graphics/Material.h>
#include <Graphics/Texture2D.h>

#include <Utils/Conversions.h>

namespace
{
	// Rough upper bound on the render command arena used by a submesh's commands, excluding the
	// vertex/index data itself (buffer creation commands + material constant buffer + upload).
	constexpr size_t k_submeshCommandOverheadBytes = 1024;
}

StaticMeshResource::StaticMeshResource()
{
}

StaticMeshResource::~StaticMeshResource()
{
}

bool StaticMeshResource::Load(const Filepath& filePath)
{
	OPTICK_EVENT("StaticMeshResource::Load");

	if (!LoadCPU(filePath))
	{
		return false;
	}

	while (!FinalizeStep())
	{
	}

	return true;
}

bool StaticMeshResource::LoadCPU(const Filepath& filePath)
{
	OPTICK_EVENT("StaticMeshResource::LoadCPU");

	MeshAssetImporter meshImporter;
	if (!meshImporter.Import(filePath))
	{
		return false;
	}

	std::vector<MeshAssetImporter::ImportedSubmesh>& importedSubmeshes = meshImporter.GetSubmeshes();

	m_pendingSubmeshes.clear();
	m_pendingSubmeshes.reserve(importedSubmeshes.size());
	for (MeshAssetImporter::ImportedSubmesh& importedSubmesh : importedSubmeshes)
	{
		PendingSubmesh& pendingSubmesh = m_pendingSubmeshes.emplace_back();
		pendingSubmesh.InterleavedVertexData = MeshGeometry::BuildInterleavedVertexData(importedSubmesh.Vertices);
		pendingSubmesh.Data = std::move(importedSubmesh);
	}

	m_finalizedSubmeshes.clear();
	m_finalizedSubmeshes.reserve(m_pendingSubmeshes.size());
	m_nextSubmeshToFinalize = 0;

	m_meshName = Conversions::StripAssetNameFromFilePath(filePath);
	m_isFinalizePending = true;

	return true;
}

void StaticMeshResource::RequestDependencies(AsyncResourceBatch& batch) const
{
	for (const PendingSubmesh& pendingSubmesh : m_pendingSubmeshes)
	{
		for (const std::string& texturePath : pendingSubmesh.Data.Material.TexturePaths)
		{
			if (!texturePath.empty())
			{
				batch.Request<Texture2D>(Filepath(texturePath));
			}
		}
	}
}

ResourceFinalizeCost StaticMeshResource::GetNextFinalizeStepCost() const
{
	ResourceFinalizeCost cost;
	if (m_nextSubmeshToFinalize < m_pendingSubmeshes.size())
	{
		const PendingSubmesh& pendingSubmesh = m_pendingSubmeshes[m_nextSubmeshToFinalize];
		const size_t dataBytes = pendingSubmesh.InterleavedVertexData.size() * sizeof(float)
			+ pendingSubmesh.Data.Indices.size() * sizeof(U32);

		cost.ArenaBytes = dataBytes + k_submeshCommandOverheadBytes;
		cost.UploadBytes = dataBytes;
	}

	return cost;
}

bool StaticMeshResource::FinalizeStep()
{
	if (!m_isFinalizePending)
	{
		return true;
	}

	if (m_nextSubmeshToFinalize < m_pendingSubmeshes.size())
	{
		PendingSubmesh& pendingSubmesh = m_pendingSubmeshes[m_nextSubmeshToFinalize];

		MeshGeometry& geometry = m_finalizedSubmeshes.emplace_back();
		geometry.SetName(pendingSubmesh.Data.Name);
		geometry.SetVertexData(std::move(pendingSubmesh.Data.Vertices));
		geometry.SetIndexData(std::move(pendingSubmesh.Data.Indices));
		// Texture lookups are cache hits when loaded through AsyncResourceBatch, since RequestDependencies
		// guarantees the textures are finalized first.
		geometry.SetMaterial(MaterialInstance::Create(pendingSubmesh.Data.Material));
		geometry.AllocateData(std::move(pendingSubmesh.InterleavedVertexData));

		++m_nextSubmeshToFinalize;
	}

	if (m_nextSubmeshToFinalize < m_pendingSubmeshes.size())
	{
		return false;
	}

	m_mesh.Initialize(m_finalizedSubmeshes);
	m_mesh.SetName(m_meshName);

	m_pendingSubmeshes.clear();
	m_finalizedSubmeshes.clear();
	m_nextSubmeshToFinalize = 0;
	m_isFinalizePending = false;

	return true;
}

void StaticMeshResource::Release()
{
}
