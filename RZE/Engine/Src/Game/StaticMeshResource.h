#pragma once

#include <Asset/AssetImport/MeshAssetImporter.h>

#include <Graphics/StaticMesh.h>
#include <Graphics/StaticMeshInstance.h>

#include <memory>

#include <Utils/Interfaces/Resource.h>

class StaticMeshResource : public IResource
{
public:
	StaticMeshResource();
	virtual ~StaticMeshResource();

public:
	bool Load(const Filepath& filePath) override;
	void Release() override;

	bool SupportsAsyncLoad() const override { return true; }
	bool LoadCPU(const Filepath& filePath) override;
	void RequestDependencies(AsyncResourceBatch& batch) const override;
	ResourceFinalizeCost GetNextFinalizeStepCost() const override;
	// Finalizes one submesh per step (GPU buffers), then the materials with the last.
	bool FinalizeStep() override;
	// The share of its submeshes finalized
	float GetFinalizeProgress() const override;

public:
	// Null until it has loaded
	const std::shared_ptr<const StaticMesh>& GetMesh() const { return m_mesh; }

	// A new instance of the loaded mesh meshResource refers to, holding meshResource so the mesh stays loaded while the
	// instance lives. Null if meshResource isn't a loaded StaticMeshResource.
	static std::shared_ptr<StaticMeshInstance> CreateInstance(const ResourceHandle& meshResource);

private:
	struct PendingSubmesh
	{
		MeshAssetImporter::ImportedSubmesh Data;
		std::vector<float> InterleavedVertexData;
	};

private:
	// Shared with every instance made from it
	std::shared_ptr<const StaticMesh> m_mesh;
	std::string m_meshName;

	// Populated by LoadCPU, consumed by FinalizeStep.
	std::vector<PendingSubmesh> m_pendingSubmeshes;
	std::vector<MeshAssetImporter::ImportedMaterial> m_pendingMaterials;
	std::vector<StaticMesh::SubMesh> m_finalizedSubmeshes;
	std::vector<MeshPlacement> m_pendingPlacements;
	size_t m_nextSubmeshToFinalize = 0;
	bool m_isFinalizePending = false;
};
