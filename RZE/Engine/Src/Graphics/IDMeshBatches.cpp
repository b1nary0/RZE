#include <Graphics/IDMeshBatches.h>

#include <Graphics/Material.h>
#include <Graphics/MaterialDefines.h>
#include <Graphics/MaterialSlot.h>
#include <Graphics/MeshDraw.h>
#include <Graphics/MeshGeometry.h>

#include <Rendering/Renderer.h>

void IDMeshBatches::Initialize()
{
	m_batches.Initialize();
}

void IDMeshBatches::Clear()
{
	m_batches.Clear();
}

void IDMeshBatches::Add(const MeshDraw& draw, U64 id)
{
	const MaterialInstance& materialInstance = draw.Slot->GetMaterial();

	// As DepthOnlyMeshBatches::Add: without an opacity map, nothing the pixel shader reads differs by material
	m_batches.Add(draw, materialInstance.HasOpacityMap() ? &materialInstance : nullptr, id);
}

U32 IDMeshBatches::Draw()
{
	m_batches.Upload();
	m_batches.BindInstances();

	for (const MeshBatches::Batch& batch : m_batches.GetBatches())
	{
		const MeshDraw& draw = *batch.Draw;
		const MaterialInstance& materialInstance = draw.Slot->GetMaterial();

		// Says whether there's a mask to clip by, so it's bound for every batch
		materialInstance.Bind({ TEXTURE_SLOT_OPACITY });

		draw.Geometry->Draw(batch.InstanceCount, batch.FirstInstance);
	}

	return static_cast<U32>(m_batches.GetBatches().size());
}
