#include <Graphics/DepthOnlyMeshBatches.h>

#include <Graphics/Material.h>
#include <Graphics/MaterialDefines.h>
#include <Graphics/MaterialSlot.h>
#include <Graphics/MeshDraw.h>
#include <Graphics/MeshGeometry.h>
#include <Graphics/Shader.h>

#include <Rendering/Renderer.h>

void DepthOnlyMeshBatches::Initialize()
{
	m_cutoutShader = LoadPixelShader("Pixel_DepthOnlyCutout");

	m_batches.Initialize();
}

void DepthOnlyMeshBatches::Clear()
{
	m_batches.Clear();
}

void DepthOnlyMeshBatches::Add(const MeshDraw& draw)
{
	const MaterialInstance& materialInstance = draw.Slot->GetMaterial();

	// Without an opacity map, only the geometry is bound, so draws of it share a batch whatever their material.
	// With one, its material's opacity is bound too.
	m_batches.Add(draw, materialInstance.HasOpacityMap() ? &materialInstance : nullptr);
}

U32 DepthOnlyMeshBatches::Draw()
{
	m_batches.Upload();
	m_batches.BindInstances();

	// Opaque draws need no pixel shader, so it's only set when a cutout's turn comes, and unset after
	bool isCutoutShaderBound = false;
	Rendering::Renderer::UnsetPixelShader();

	for (const MeshBatches::Batch& batch : m_batches.GetBatches())
	{
		const MeshDraw& draw = *batch.Draw;
		const MaterialInstance& materialInstance = draw.Slot->GetMaterial();

		const bool isCutout = materialInstance.HasOpacityMap();
		if (isCutout != isCutoutShaderBound)
		{
			if (isCutout)
			{
				Rendering::Renderer::SetPixelShader(m_cutoutShader->GetPlatformObject());
			}
			else
			{
				Rendering::Renderer::UnsetPixelShader();
			}
			isCutoutShaderBound = isCutout;
		}

		if (isCutout)
		{
			materialInstance.Bind({ TEXTURE_SLOT_OPACITY });
		}

		draw.Geometry->Draw(batch.InstanceCount, batch.FirstInstance);
	}

	return static_cast<U32>(m_batches.GetBatches().size());
}
