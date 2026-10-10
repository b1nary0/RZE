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

void LightObject::Initialize()
{
	m_propertyBuffer = Rendering::Renderer::CreateConstantBuffer(nullptr, sizeof(PropertyBufferLayout), 16, 1);
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

#ifdef _DEBUG
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
