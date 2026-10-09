#include <StdAfx.h>
#include <Graphics/RenderEngine.h>

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

void RenderEngine::Update()
{
	OPTICK_EVENT();

	for (auto& pipeline : m_renderStages)
	{
		pipeline->Update(m_camera, m_sceneData);
	}
}

void RenderEngine::Render(const char* frameName, bool isMainRenderCall, bool withImgui)
{
	OPTICK_EVENT();

	if (isMainRenderCall)
	{
		Rendering::Renderer::BeginFrame(frameName);
	}

	for (auto& pipeline : m_renderStages)
	{
		// #TODO dont do imgui. magic number should change
		if (!withImgui && pipeline->GetPriority() == 1000)
		{
			continue;
		}

		pipeline->Render(m_camera, m_sceneData);
	}

	m_sceneData.debugLines.clear();

	if (isMainRenderCall)
	{
		Rendering::Renderer::EndFrame();
	}
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
}

void RenderEngine::ReleaseRenderStages()
{
	m_renderStages.clear();
}

RenderObjectPtr RenderEngine::CreateRenderObject(const StaticMeshInstance& staticMesh)
{
	OPTICK_EVENT();
	
	std::unique_ptr<RenderObject>& renderObjectPtr = m_sceneData.renderObjects.emplace_back(std::make_unique<RenderObject>());
	renderObjectPtr->SetStaticMesh(staticMesh);
	
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
	}
}

LightObjectPtr RenderEngine::CreateLightObject()
{
	std::unique_ptr<LightObject>& lightObjectPtr = m_sceneData.lightObjects.emplace_back(std::make_unique<LightObject>());
	lightObjectPtr->Initialize();
	
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

const Rendering::RenderTargetTexture& RenderEngine::GetRenderTarget()
{
	AssertNotNull(m_renderTarget);
	return *m_renderTarget;
}

struct RenderViewData
{
	RenderCamera Camera;

};

void RenderEngine::RenderView(const char* frameName, const RenderCamera& renderCamera, std::unique_ptr<Rendering::RenderTargetTexture>& renderTarget)
{
	OPTICK_EVENT();
	AssertNotNull(renderTarget);
	AssertExpr(renderCamera.Viewport.Size != Vector2D::ZERO);

	const RenderCamera prevCamera = GetCamera();
	const Vector2D prevViewportSize = GetViewportSize();
	Rendering::RenderTargetTexture* prevRenderTarget = m_renderTarget;

	GetCamera() = renderCamera; // This bad
	SetViewportSize(renderCamera.Viewport.Size);
	SetRenderTarget(renderTarget.get());

	Update();
	m_isRenderingMainView = false;
	Render(frameName, false, false);
	m_isRenderingMainView = true;

	SetViewportSize(prevViewportSize);
	SetRenderTarget(prevRenderTarget);
	GetCamera() = prevCamera;
}

void RenderEngine::InternalAddRenderStage(IRenderStage* pipeline)
{
	std::unique_ptr<IRenderStage> ptr = std::unique_ptr<IRenderStage>(pipeline);
	ptr->Initialize();
	m_renderStages.emplace_back(std::move(ptr));

	std::sort(m_renderStages.begin(), m_renderStages.end(),
		[](const std::unique_ptr<IRenderStage>& pipelineA, const std::unique_ptr<IRenderStage>& pipelineB)
		{
			return pipelineA->GetPriority() < pipelineB->GetPriority();
		});
}
