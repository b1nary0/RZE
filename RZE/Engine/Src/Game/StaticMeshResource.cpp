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
	m_pendingMaterials = std::move(meshImporter.GetMaterials());
	m_pendingPlacements = std::move(meshImporter.GetPlacements());

	m_finalizedSubmeshes.clear();
	m_finalizedSubmeshes.reserve(m_pendingSubmeshes.size());
	m_nextSubmeshToFinalize = 0;

	m_meshName = Conversions::StripAssetNameFromFilePath(filePath);
	m_isFinalizePending = true;

	return true;
}

void StaticMeshResource::RequestDependencies(AsyncResourceBatch& batch) const
{
	for (const MeshAssetImporter::ImportedMaterial& pendingMaterial : m_pendingMaterials)
	{
		for (const std::string& texturePath : pendingMaterial.Data.TexturePaths)
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

		StaticMesh::SubMesh& subMesh = m_finalizedSubmeshes.emplace_back();
		subMesh.SlotIndex = pendingSubmesh.Data.MaterialIndex;
		MeshGeometry& geometry = subMesh.Geometry;
		geometry.SetName(pendingSubmesh.Data.Name);
		geometry.SetVertexData(std::move(pendingSubmesh.Data.Vertices));
		geometry.SetIndexData(std::move(pendingSubmesh.Data.Indices));
		geometry.AllocateData(std::move(pendingSubmesh.InterleavedVertexData));

		++m_nextSubmeshToFinalize;
	}

	if (m_nextSubmeshToFinalize < m_pendingSubmeshes.size())
	{
		return false;
	}

	// One material per material asset, shared by the submeshes using it. Texture lookups are cache hits when loaded
	// through AsyncResourceBatch, since RequestDependencies guarantees the textures are finalized first.
	std::vector<StaticMesh::Slot> slots;
	slots.reserve(m_pendingMaterials.size());
	for (const MeshAssetImporter::ImportedMaterial& pendingMaterial : m_pendingMaterials)
	{
		StaticMesh::Slot& slot = slots.emplace_back();
		slot.Name = pendingMaterial.Name;
		slot.Material = MaterialInstance::Create(pendingMaterial.Data);
	}

	m_mesh = StaticMesh::Create(m_meshName, std::move(m_finalizedSubmeshes), std::move(slots), std::move(m_pendingPlacements));

	m_pendingSubmeshes.clear();
	m_pendingMaterials.clear();
	m_pendingPlacements.clear();
	m_finalizedSubmeshes.clear();
	m_nextSubmeshToFinalize = 0;
	m_isFinalizePending = false;

	return true;
}

void StaticMeshResource::Release()
{
	// Instances made from it keep the mesh alive until they go
	m_mesh.reset();
}

std::shared_ptr<StaticMeshInstance> StaticMeshResource::CreateInstance(const ResourceHandle& meshResource)
{
	if (!meshResource.IsValid())
	{
		return nullptr;
	}

	const StaticMeshResource* const resource = RZE().GetResourceHandler().GetResource<StaticMeshResource>(meshResource);
	if (resource == nullptr || resource->GetMesh() == nullptr)
	{
		return nullptr;
	}

	// The instance holds its own handle; copying one takes another reference
	ResourceHandle heldResource;
	heldResource = meshResource;
	return StaticMeshInstance::Create(resource->GetMesh(), std::move(heldResource));
}
