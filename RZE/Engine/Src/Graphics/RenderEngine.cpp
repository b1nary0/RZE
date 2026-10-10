#include <StdAfx.h>
#include <Graphics/RenderEngine.h>

#include <Graphics/RenderPipeline.h>
#include <Graphics/RenderStage.h>
#include <Graphics/RenderStages/DebugDrawRenderStage.h>
#include <Graphics/RenderStages/ForwardRenderStage.h>
#include <Graphics/RenderStages/PostProcessRenderStage.h>
#include <Graphics/RenderStages/ShadowRenderStage.h>

#include <Rendering/Renderer.h>

#include "Rendering/Graphics/RenderTarget.h"

#include <Utils/Math/Math.h>

#include <cfloat>

void LightObject::Initialize()
{
	m_propertyBuffer = Rendering::Renderer::CreateConstantBuffer(nullptr, sizeof(PropertyBufferLayout), 16, 1);
}

void RenderObject::SetStaticMesh(const StaticMeshInstance& staticMesh)
{
	m_staticMesh = staticMesh;
	UpdateWorldBounds();
	MarkSceneChanged();
}

void RenderObject::SetTransform(const Matrix4x4& transform)
{
	if (transform != m_matrixMem.transform)
	{
		m_matrixMem.transform = transform;
		m_matrixMem.invTransform = transform.Inverse();
		UpdateWorldBounds();
		MarkSceneChanged();
	}
}

void RenderObject::UpdateWorldBounds()
{
	const std::vector<MeshGeometry>& subMeshes = static_cast<const StaticMeshInstance&>(m_staticMesh).GetSubMeshes();
	const Matrix4x4& transform = m_matrixMem.transform;

	m_subMeshBounds.resize(subMeshes.size());

	Vector3D objectMin(FLT_MAX);
	Vector3D objectMax(-FLT_MAX);

	for (size_t subMeshIndex = 0; subMeshIndex < subMeshes.size(); ++subMeshIndex)
	{
		const Vector3D& localMin = subMeshes[subMeshIndex].GetBoundsMin();
		const Vector3D& localMax = subMeshes[subMeshIndex].GetBoundsMax();
		SubMeshBounds& bounds = m_subMeshBounds[subMeshIndex];

		bounds.Min = Vector3D(FLT_MAX);
		bounds.Max = Vector3D(-FLT_MAX);

		// Corner i takes max on X if bit 0 is set, Y if bit 1, Z if bit 2
		for (int corner = 0; corner < 8; ++corner)
		{
			const Vector3D localCorner(
				(corner & 1) ? localMax.X() : localMin.X(),
				(corner & 2) ? localMax.Y() : localMin.Y(),
				(corner & 4) ? localMax.Z() : localMin.Z());

			bounds.Corners[corner] = (transform * Vector4D(localCorner, 1.0f)).XYZ();
			bounds.Min = VectorUtils::Min(bounds.Min, bounds.Corners[corner]);
			bounds.Max = VectorUtils::Max(bounds.Max, bounds.Corners[corner]);
		}

		objectMin = VectorUtils::Min(objectMin, bounds.Min);
		objectMax = VectorUtils::Max(objectMax, bounds.Max);
	}

	if (subMeshes.empty())
	{
		objectMin = Vector3D::ZERO;
		objectMax = Vector3D::ZERO;
	}

	m_boundsMin = objectMin;
	m_boundsMax = objectMax;
}

RenderEngine::RenderEngine()
	: m_pipeline(std::make_unique<RenderPipeline>())
{
}

RenderEngine::~RenderEngine()
{
}

void RenderEngine::Initialize(void* windowHandle)
{
	Rendering::Renderer::Initialize(windowHandle);
	
	AddRenderStage<ShadowRenderStage>();
	AddRenderStage<ForwardRenderStage>();
	AddRenderStage<PostProcessRenderStage>();

	// Available in Debug and Release; only Retail strips it
#ifndef RZE_RETAIL
	AddRenderStage<DebugDrawRenderStage>();
#endif
}

void RenderEngine::Render(const char* frameName)
{
	OPTICK_EVENT();
	AssertNotNull(m_mainView.Target);

	Rendering::Renderer::BeginFrame(frameName);

	m_pipeline->Render(m_mainView, m_sceneData, static_cast<float>(RZE().GetDeltaTime()));

	// Secondary views rendered earlier this frame have drawn them too
	m_sceneData.debugLines.clear();

	Rendering::Renderer::EndFrame();
}

void RenderEngine::Finish()
{
	OPTICK_EVENT();
	Rendering::Renderer::DevicePresent();
}

void RenderEngine::Shutdown()
{
	Rendering::Renderer::Shutdown();
}

void RenderEngine::ClearObjects()
{
	m_sceneData.renderObjects.clear();
	++m_sceneData.revision;
}

void RenderEngine::ReleaseRenderStages()
{
	m_pipeline->Clear();
}

RenderObjectPtr RenderEngine::CreateRenderObject(const StaticMeshInstance& staticMesh)
{
	OPTICK_EVENT();
	
	std::unique_ptr<RenderObject>& renderObjectPtr = m_sceneData.renderObjects.emplace_back(std::make_unique<RenderObject>());
	renderObjectPtr->m_sceneRevision = &m_sceneData.revision;
	renderObjectPtr->SetStaticMesh(staticMesh);
	++m_sceneData.revision;

	return RenderObjectPtr(renderObjectPtr.get());
}

void RenderEngine::DestroyRenderObject(RenderObjectPtr& renderObject)
{
	OPTICK_EVENT();

	RenderObjectContainer& renderObjects = m_sceneData.renderObjects;
	auto iter = std::find_if(renderObjects.begin(), renderObjects.end(),
		[&renderObject](const std::unique_ptr<RenderObject>& other)
		{
			return renderObject.m_ptr == other.get();
		});

	if (iter != renderObjects.end())
	{
		if (renderObjects.size() > 1)
		{
			std::iter_swap(iter, std::prev(renderObjects.end()));
			renderObjects.erase(std::prev(renderObjects.end()));
		}
		else
		{
			renderObjects.erase(iter);
		}

		renderObject = RenderObjectPtr();
		++m_sceneData.revision;
	}
}

LightObjectPtr RenderEngine::CreateLightObject()
{
	std::unique_ptr<LightObject>& lightObjectPtr = m_sceneData.lightObjects.emplace_back(std::make_unique<LightObject>());
	lightObjectPtr->m_sceneRevision = &m_sceneData.revision;
	lightObjectPtr->Initialize();
	++m_sceneData.revision;

	return LightObjectPtr(lightObjectPtr.get());
}

void RenderEngine::DestroyLightObject(LightObjectPtr& lightObject)
{
	OPTICK_EVENT();

	LightObjectContainer& lightObjects = m_sceneData.lightObjects;
	auto iter = std::find_if(lightObjects.begin(), lightObjects.end(),
		[&lightObject](const std::unique_ptr<LightObject>& other)
		{
			return lightObject.m_ptr == other.get();
		});

	if (iter != lightObjects.end())
	{
		if (lightObjects.size() > 1)
		{
			std::iter_swap(iter, std::prev(lightObjects.end()));
			lightObjects.erase(std::prev(lightObjects.end()));
		}
		else
		{
			lightObjects.erase(iter);
		}

		lightObject = LightObjectPtr();
		++m_sceneData.revision;
	}
}

void RenderEngine::DrawLine(const Vector3D& start, const Vector3D& end, const Vector3D& colour)
{
	DebugLine line{ start, end, colour };
	m_sceneData.debugLines.emplace_back(std::move(line));
}

void RenderEngine::DrawWireBox(const Vector3D& boundsMin, const Vector3D& boundsMax, const Matrix4x4& transform, const Vector3D& colour)
{
	// Corner i takes max on X if bit 0 is set, Y if bit 1, Z if bit 2.
	// Transformed individually so the box follows the object's rotation and scale.
	Vector3D corners[8];
	for (int i = 0; i < 8; ++i)
	{
		const Vector4D corner = transform * Vector4D(
			(i & 1) ? boundsMax.X() : boundsMin.X(),
			(i & 2) ? boundsMax.Y() : boundsMin.Y(),
			(i & 4) ? boundsMax.Z() : boundsMin.Z(),
			1.0f);
		corners[i] = Vector3D(corner.X(), corner.Y(), corner.Z());
	}

	// Edges connect corners that differ by exactly one axis bit
	for (int i = 0; i < 8; ++i)
	{
		for (int axisBit = 1; axisBit < 8; axisBit <<= 1)
		{
			if ((i & axisBit) == 0)
			{
				DrawLine(corners[i], corners[i | axisBit], colour);
			}
		}
	}
}

void RenderEngine::ResizeCanvas(const Vector2D& newSize)
{
	m_canvasSize = newSize;

	Rendering::Renderer::HandleWindowResize(m_canvasSize);

	ImGui::GetIO().DisplaySize.x = m_canvasSize.X();
	ImGui::GetIO().DisplaySize.y = m_canvasSize.Y();

	RZE_LOG_ARGS("New Canvas Size: %f x %f", m_canvasSize.X(), m_canvasSize.Y());
}

const Vector2D& RenderEngine::GetCanvasSize() const
{
	return m_canvasSize;
}

void RenderEngine::RenderSecondaryView(const char* frameName, RenderView& view)
{
	OPTICK_EVENT();
	AssertExpr(view.GetKind() == ERenderViewKind::Secondary);
	AssertNotNull(view.Target);
	AssertExpr(view.ViewportSize != Vector2D::ZERO);

	Rendering::Renderer::Begin(frameName);

	m_pipeline->Render(view, m_sceneData, static_cast<float>(RZE().GetDeltaTime()));

	Rendering::Renderer::End();
}

void RenderEngine::InternalAddRenderStage(IRenderStage* stage)
{
	m_pipeline->AddStage(std::unique_ptr<IRenderStage>(stage));
}
