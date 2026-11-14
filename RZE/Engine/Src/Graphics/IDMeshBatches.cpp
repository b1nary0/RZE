#include <Graphics/IDMeshBatches.h>

#include <Graphics/IndexBuffer.h>
#include <Graphics/Material.h>
#include <Graphics/MaterialDefines.h>
#include <Graphics/MaterialSlot.h>
#include <Graphics/MeshDraw.h>
#include <Graphics/MeshGeometry.h>
#include <Graphics/Texture2D.h>
#include <Graphics/VertexBuffer.h>

#include <Rendering/Renderer.h>

namespace
{
	// Where the pixel shaders read the material's buffer (MaterialBuffer in Common/PixelResources.hlsli)
	constexpr U32 k_materialBufferSlot = 1;
}

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
	const bool isCutout = materialInstance.GetTextureResource(TEXTURE_SLOT_OPACITY) != nullptr;
	m_batches.Add(draw, isCutout ? &materialInstance : nullptr, id);
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
		Rendering::Renderer::SetConstantBufferPS(materialInstance.GetParamBuffer(), k_materialBufferSlot);
		if (const Texture2D* const opacityMap = materialInstance.GetTextureResource(TEXTURE_SLOT_OPACITY))
		{
			Rendering::Renderer::SetTextureResource(opacityMap->GetPlatformObject(), TEXTURE_SLOT_OPACITY);
		}

		const MeshGeometry& meshGeometry = *draw.Geometry;
		const Rendering::IndexBufferHandle& indexBuffer = meshGeometry.GetIndexBuffer()->GetPlatformObject();
		Rendering::Renderer::SetVertexBuffer(meshGeometry.GetVertexBuffer()->GetPlatformObject(), 0);
		Rendering::Renderer::SetIndexBuffer(indexBuffer);

		Rendering::Renderer::DrawIndexedInstanced(indexBuffer, batch.InstanceCount, batch.FirstInstance);
	}

	return static_cast<U32>(m_batches.GetBatches().size());
}
