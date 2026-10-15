#pragma once

#include <Asset/AssetImport/MaterialAssetImporter.h>

#include <Utils/Interfaces/Resource.h>

#include <memory>

class MaterialInstance;

// A material asset loaded on its own, rather than as part of a mesh: one material, shared by everything that draws
// with the asset (e.g. material slots that swap it in through MaterialOverrides::Material). Loads asynchronously, its
// textures first.
class MaterialResource : public IResource
{
public:
	MaterialResource();
	~MaterialResource() override;

public:
	bool Load(const Filepath& filePath) override;
	void Release() override;

	bool SupportsAsyncLoad() const override { return true; }
	bool LoadCPU(const Filepath& filePath) override;
	void RequestDependencies(AsyncResourceBatch& batch) const override;
	// Makes the material, in one step
	bool FinalizeStep() override;

public:
	// Null until it has loaded
	const std::shared_ptr<const MaterialInstance>& GetMaterial() const { return m_material; }

private:
	// Read by LoadCPU, made into the material by FinalizeStep
	MaterialAssetImporter::MaterialData m_pendingData;
	bool m_isFinalizePending = false;

	std::shared_ptr<const MaterialInstance> m_material;
};
