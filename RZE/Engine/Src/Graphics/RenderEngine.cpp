#include <Graphics/RenderEngine.h>

#include <Graphics/RenderPipeline.h>
#include <Graphics/RenderStage.h>
#include <Graphics/RenderStages/AmbientOcclusionStage.h>
#include <Graphics/RenderStages/AntiAliasingStage.h>
#include <Graphics/RenderStages/DebugDrawRenderStage.h>
#include <Graphics/RenderStages/DebugViewStage.h>
#include <Graphics/RenderStages/DepthPrepassStage.h>
#include <Graphics/RenderStages/ForwardRenderStage.h>
#include <Graphics/RenderStages/IrradianceProbeStage.h>
#include <Graphics/RenderStages/PickRenderStage.h>
#include <Graphics/RenderStages/PostProcessRenderStage.h>
#include <Graphics/RenderStages/ReflectionProbeStage.h>
#include <Graphics/RenderStages/ShadowRenderStage.h>

#include <Rendering/Renderer.h>

#include <Utils/DebugUtils/DebugConfig.h>
#include <Utils/TimeUtils.h>

#include "Rendering/Graphics/RenderTarget.h"

#ifdef RZE_RENDER_STATS
namespace
{
	RenderWorkCounts PerFrame(const StatsWindow& window, const RenderWorkCounts& totals)
	{
		RenderWorkCounts perFrame;
		perFrame.IrradianceProbesCaptured = window.PerFrame(totals.IrradianceProbesCaptured);
		perFrame.IrradianceCaptureDraws = window.PerFrame(totals.IrradianceCaptureDraws);
		perFrame.IrradianceProbesRelit = window.PerFrame(totals.IrradianceProbesRelit);
		perFrame.ReflectionProbesCaptured = window.PerFrame(totals.ReflectionProbesCaptured);
		perFrame.ReflectionCaptureDraws = window.PerFrame(totals.ReflectionCaptureDraws);
		perFrame.ReflectionProbesRelit = window.PerFrame(totals.ReflectionProbesRelit);
		perFrame.SunShadowMapsRendered = window.PerFrame(totals.SunShadowMapsRendered);
		perFrame.PointShadowFacesRendered = window.PerFrame(totals.PointShadowFacesRendered);
		perFrame.ShadowDraws = window.PerFrame(totals.ShadowDraws);
		return perFrame;
	}
}
#endif

RenderEngine::RenderEngine()
	: m_pipeline(std::make_unique<RenderPipeline>())
{
}

RenderEngine::~RenderEngine()
{
}

void RenderEngine::Initialize(void* windowHandle, const Rendering::GFXDeviceOptions& deviceOptions)
{
	Rendering::Renderer::Initialize(windowHandle, deviceOptions);
	
	AddRenderStage<ShadowRenderStage>();
	AddRenderStage<DepthPrepassStage>();
	AddRenderStage<AmbientOcclusionStage>();
	AddRenderStage<IrradianceProbeStage>(&m_globalIlluminationSettings);
	AddRenderStage<ReflectionProbeStage>(&m_globalIlluminationSettings);
	AddRenderStage<ForwardRenderStage>();
	AddRenderStage<PostProcessRenderStage>();
	AddRenderStage<AntiAliasingStage>();
	// Before the debug lines, which it would otherwise cover; both modify DisplayColourData, in the order they're added
	AddRenderStage<DebugViewStage>();
	AddRenderStage<PickRenderStage>();

#ifdef RZE_DEBUG_DRAW_ENABLED
	AddRenderStage<DebugDrawRenderStage>();
#endif
}

void RenderEngine::Render(const char* frameName)
{
	OPTICK_EVENT();
	AssertNotNull(m_mainView.Target);

#ifdef RZE_RENDER_STATS
	// Before recording, so a frame's and a window's stage times and draw calls are their own frames'
	const double renderStart = TimeUtils::GetSeconds();
	EndFrameCounts();
	UpdateStats(renderStart);
	ApplyGPUStatisticsRequest();
#endif

	Rendering::Renderer::BeginFrame(frameName);

	m_pipeline->Render(m_mainView, GetMainScene(), static_cast<float>(RZE().GetDeltaTime()), m_frameIndex);
	++m_frameIndex;

	// Secondary views rendered earlier this frame have drawn them too
	GetMainScene().ClearDebugLines();
	m_mainView.ClearFrameRequests();

	Rendering::Renderer::EndFrame();

#ifdef RZE_RENDER_STATS
	m_frameRecordTime += TimeUtils::GetSeconds() - renderStart;
#endif
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
	// The main view and scene live until the render engine is destroyed, after the renderer has shut down, so what the
	// stages keep in them goes now
	m_mainView.ReleaseStageState();
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

#ifdef RZE_RENDER_STATS
	const double renderStart = TimeUtils::GetSeconds();
#endif

	Rendering::Renderer::Begin(frameName);

	m_pipeline->Render(view, scene, static_cast<float>(RZE().GetDeltaTime()), m_frameIndex);
	view.ClearFrameRequests();

	Rendering::Renderer::End();

#ifdef RZE_RENDER_STATS
	m_frameRecordTime += TimeUtils::GetSeconds() - renderStart;
#endif
}

void RenderEngine::InternalAddRenderStage(IRenderStage* stage)
{
	m_pipeline->AddStage(std::unique_ptr<IRenderStage>(stage));
}

#ifdef RZE_RENDER_STATS
void RenderEngine::EndFrameCounts()
{
	m_lastFrame.RecordTimeMS = m_frameRecordTime * 1000.0;
	const Rendering::DrawCounts frameDraws = Rendering::Renderer::TakeDrawCounts();
	m_lastFrame.DrawCalls = frameDraws.DrawCalls;
	m_lastFrame.Instances = frameDraws.Instances;

	m_windowRecordTime += m_frameRecordTime;
	m_windowDraws += frameDraws;
	m_frameRecordTime = 0.0;
}

void RenderEngine::UpdateStats(double now)
{
	if (!m_statsWindow.AddFrame(now))
	{
		return;
	}

	// The stage stats build up over the window, so they're taken once at its end
	m_stats.RecordTimeMS = m_statsWindow.PerFrame(m_windowRecordTime) * 1000.0;
	m_stats.DrawCalls = m_statsWindow.PerFrame(static_cast<double>(m_windowDraws.DrawCalls));
	m_stats.Instances = m_statsWindow.PerFrame(static_cast<double>(m_windowDraws.Instances));
	m_stats.Work = PerFrame(m_statsWindow, m_pipeline->TakeWorkCounts());

	// Already per frame: the render thread averages them over its own window
	const Rendering::RenderThreadStats renderThreadStats = Rendering::Renderer::GetRenderThreadStats();
	m_stats.CanCountGPUStatistics = renderThreadStats.CanCountGPUStatistics;
	m_stats.HasGPUStatistics = renderThreadStats.HasGPUStatistics;
	m_stats.GPUStatistics = renderThreadStats.GPUFrameStatistics;

	m_stats.Stages.clear();
	for (RenderStageStats& stageStats : m_pipeline->TakeStageStats())
	{
		// The time, draws and state changes are totals over this window; the GPU's figures are already per frame
		stageStats.TimeMS = m_statsWindow.PerFrame(stageStats.TimeMS);
		stageStats.DrawCalls = m_statsWindow.PerFrame(stageStats.DrawCalls);
		stageStats.Instances = m_statsWindow.PerFrame(stageStats.Instances);
		stageStats.StateChanges = Rendering::ToPerFrame(stageStats.StateChanges, m_statsWindow.GetFrameCount());
		m_stats.Stages.push_back(std::move(stageStats));
	}

	m_windowRecordTime = 0.0;
	m_windowDraws = Rendering::DrawCounts();
}

void RenderEngine::ApplyGPUStatisticsRequest()
{
	const bool isRequested = m_isGPUStatisticsRequested;
	m_isGPUStatisticsRequested = false;
	if (isRequested != m_areGPUStatisticsOn)
	{
		Rendering::Renderer::SetGPUStatistics(isRequested);
		m_areGPUStatisticsOn = isRequested;
	}
}
#endif
