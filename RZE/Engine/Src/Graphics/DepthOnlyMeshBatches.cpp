#include <Graphics/DepthOnlyMeshBatches.h>

#include <Graphics/Material.h>
#include <Graphics/MaterialDefines.h>
#include <Graphics/MaterialSlot.h>
#include <Graphics/MeshDraw.h>
#include <Graphics/MeshGeometry.h>
#include <Graphics/MeshInstanceFormat.h>
#include <Graphics/Shader.h>

#include <Rendering/Renderer.h>

void DepthOnlyMeshBatches::Initialize()
{
	// The same instances, so one stream feeds both (MeshInstanceFormats::DepthPosition)
	m_opaqueVertexShader = LoadVertexShader("Vertex_MeshDepthOnlyPosition", MeshInstanceFormats::DepthPosition().InputLayout);
	m_cutoutVertexShader = LoadVertexShader("Vertex_MeshDepthOnly", MeshInstanceFormats::Depth().InputLayout);
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
	// With one, its material's opacity is bound too, with the cutout shader, so the cutouts are drawn together.
	if (materialInstance.HasOpacityMap())
	{
		m_batches.Add(draw, &materialInstance, m_cutoutShader.Get());
	}
	else
	{
		m_batches.Add(draw, nullptr, nullptr);
	}
}

U32 DepthOnlyMeshBatches::Draw()
{
	m_batches.Upload();
	m_batches.BindInstances();

	// Opaque draws need no pixel shader, so it's only set when a cutout's turn comes, and unset after
	bool isCutoutBound = false;
	Rendering::Renderer::UnsetPixelShader();

	// Each vertex shader is bound with its input layout when the first batch of a run comes
	bool isVertexShaderBound = false;

	for (const MeshBatches::Batch& batch : m_batches.GetBatches())
	{
		const MeshDraw& draw = *batch.Draw;
		const MaterialInstance& materialInstance = draw.Slot->GetMaterial();

		const bool isCutout = materialInstance.HasOpacityMap();
		if (!isVertexShaderBound || isCutout != isCutoutBound)
		{
			const VertexShader& vertexShader = isCutout ? *m_cutoutVertexShader.Get() : *m_opaqueVertexShader.Get();
			Rendering::Renderer::SetVertexShader(vertexShader.GetPlatformObject());
			Rendering::Renderer::SetInputLayout(vertexShader.GetPlatformObject());
			isVertexShaderBound = true;
		}

		if (isCutout != isCutoutBound)
		{
			if (isCutout)
			{
				Rendering::Renderer::SetPixelShader(m_cutoutShader->GetPlatformObject());
			}
			else
			{
				Rendering::Renderer::UnsetPixelShader();
			}
			isCutoutBound = isCutout;
		}

		if (isCutout)
		{
			materialInstance.Bind({ TEXTURE_SLOT_OPACITY });
			draw.Geometry->Draw(batch.InstanceCount, batch.FirstInstance);
		}
		else
		{
			draw.Geometry->DrawDepth(batch.InstanceCount, batch.FirstInstance);
		}
	}

	return static_cast<U32>(m_batches.GetBatches().size());
}
