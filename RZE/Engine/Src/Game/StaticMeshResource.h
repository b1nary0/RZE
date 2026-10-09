#pragma once

#include <Asset/AssetImport/MeshAssetImporter.h>

#include <Graphics/StaticMeshInstance.h>

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
	// Finalizes one submesh per step (material + GPU buffers).
	bool FinalizeStep() override;

public:
	const StaticMeshInstance& GetStaticMesh() const { return m_mesh; }

	[[nodiscard]]
	StaticMeshInstance GetInstance() const { return m_mesh; }

private:
	struct PendingSubmesh
	{
		MeshAssetImporter::ImportedSubmesh Data;
		std::vector<float> InterleavedVertexData;
	};

private:
	StaticMeshInstance m_mesh;
	std::string m_meshName;

	// Populated by LoadCPU, consumed by FinalizeStep.
	std::vector<PendingSubmesh> m_pendingSubmeshes;
	std::vector<MeshGeometry> m_finalizedSubmeshes;
	size_t m_nextSubmeshToFinalize = 0;
	bool m_isFinalizePending = false;
};
