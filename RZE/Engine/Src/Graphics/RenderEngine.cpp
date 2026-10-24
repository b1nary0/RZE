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

#include <Utils/TimeUtils.h>

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

	const double renderStart = TimeUtils::GetSeconds();
	if (m_windowStart < 0.0)
	{
		m_windowStart = renderStart;
	}

	Rendering::Renderer::BeginFrame(frameName);

	m_pipeline->Render(m_mainView, GetMainScene(), static_cast<float>(RZE().GetDeltaTime()));

	// Secondary views rendered earlier this frame have drawn them too
	GetMainScene().ClearDebugLines();
	m_mainView.Pick.reset();

	Rendering::Renderer::EndFrame();

	const double renderEnd = TimeUtils::GetSeconds();
	m_windowRecordTime += renderEnd - renderStart;
	UpdateStats(renderEnd);
}

void RenderEngine::Finish()
{
	OPTICK_EVENT();
	Rendering::Renderer::DevicePresent();
}

void RenderEngine::Submit()
{
	OPTICK_EVENT();
	Rendering::Renderer::Submit();
}

void RenderEngine::Shutdown()
{
	Rendering::Renderer::Shutdown();
}

void RenderEngine::SetVSync(bool enabled)
{
	Rendering::Renderer::SetSyncInterval(enabled ? 1 : 0);
}

void RenderEngine::WaitForPresentSlot()
{
	OPTICK_EVENT("Wait | RenderEngine::WaitForPresentSlot");

	// Long enough for any real frame. It runs out when nothing is being presented, e.g. while minimised, and the
	// frame then goes ahead anyway.
	constexpr U32 k_timeoutMS = 100;
	Rendering::Renderer::WaitForPresentSlot(k_timeoutMS);
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

	const double renderStart = TimeUtils::GetSeconds();

	Rendering::Renderer::Begin(frameName);

	m_pipeline->Render(view, scene, static_cast<float>(RZE().GetDeltaTime()));

	Rendering::Renderer::End();

	m_windowRecordTime += TimeUtils::GetSeconds() - renderStart;
}

void RenderEngine::InternalAddRenderStage(IRenderStage* stage)
{
	m_pipeline->AddStage(std::unique_ptr<IRenderStage>(stage));
}

void RenderEngine::UpdateStats(double now)
{
	++m_windowFrames;
	if (now - m_windowStart < k_statsWindow)
	{
		return;
	}

	// The stage times and draw calls build up over the window, so they're taken once at its end
	const double frames = static_cast<double>(m_windowFrames);
	m_stats.RecordTimeMS = m_windowRecordTime / frames * 1000.0;
	m_stats.DrawCalls = static_cast<double>(Rendering::Renderer::TakeDrawCallCount()) / frames;

	m_stats.Stages.clear();
	for (const RenderPipeline::StageTime& stageTime : m_pipeline->TakeStageTimes())
	{
		m_stats.Stages.push_back({ stageTime.Name, stageTime.Seconds / frames * 1000.0 });
	}

	m_windowStart = now;
	m_windowFrames = 0;
	m_windowRecordTime = 0.0;
}
