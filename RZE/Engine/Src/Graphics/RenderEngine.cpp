#include <StdAfx.h>
#include <Graphics/RenderEngine.h>

#include <Graphics/RenderPipeline.h>
#include <Graphics/RenderStage.h>
#include <Graphics/RenderStages/DebugDrawRenderStage.h>
#include <Graphics/RenderStages/ForwardRenderStage.h>
#include <Graphics/RenderStages/PickRenderStage.h>
#include <Graphics/RenderStages/PostProcessRenderStage.h>
#include <Graphics/RenderStages/ShadowRenderStage.h>

#include <Rendering/Renderer.h>

#include "Rendering/Graphics/RenderTarget.h"

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
	AddRenderStage<PickRenderStage>();

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

	m_pipeline->Render(m_mainView, GetMainScene(), static_cast<float>(RZE().GetDeltaTime()));

	// Secondary views rendered earlier this frame have drawn them too
	GetMainScene().ClearDebugLines();
	m_mainView.Pick.reset();

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

void RenderEngine::ReleaseRenderStages()
{
	m_pipeline->Clear();
	if (m_mainScene != nullptr)
	{
		m_mainScene->ReleaseStageState();
	}
}

RenderScene& RenderEngine::GetMainScene()
{
	AssertMsg(m_mainScene != nullptr, "The main scene is set when the engine creates the active scene");
	return *m_mainScene;
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

void RenderEngine::RenderSecondaryView(const char* frameName, RenderView& view, RenderScene& scene)
{
	OPTICK_EVENT();
	AssertExpr(view.GetKind() == ERenderViewKind::Secondary);
	AssertNotNull(view.Target);
	AssertExpr(view.ViewportSize != Vector2D::ZERO);

	Rendering::Renderer::Begin(frameName);

	m_pipeline->Render(view, scene, static_cast<float>(RZE().GetDeltaTime()));

	Rendering::Renderer::End();
}

void RenderEngine::InternalAddRenderStage(IRenderStage* stage)
{
	m_pipeline->AddStage(std::unique_ptr<IRenderStage>(stage));
}
