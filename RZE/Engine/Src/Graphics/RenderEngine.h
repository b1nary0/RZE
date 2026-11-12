#pragma once

#include <Graphics/GlobalIllumination.h>
#include <Graphics/GraphicsDefines.h>
#include <Graphics/RenderPipeline.h>
#include <Graphics/RenderScene.h>
#include <Graphics/RenderView.h>
#include <Graphics/RenderWorkCounts.h>

#include <Rendering/BufferHandle.h>

#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector2D.h>
#include <Utils/Math/Vector3D.h>
#include <Utils/Math/Vector4D.h>
#include <Utils/StatsWindow.h>

#include <vector>

namespace Rendering
{
	class RenderTargetTexture;
	struct GFXDeviceOptions;
}

class IRenderStage;

// RenderEngine's frames, averaged over a StatsWindow, for display. Measured on the main thread, where the render stages
// record commands for the render thread. A frame runs from one Render() to the next.
struct RenderStats
{
	// Per frame, recording every view's render stages
	double RecordTimeMS = 0.0;
	// Each stage's part of RecordTimeMS, DrawCalls, Instances and GPUStatistics per frame, in the order they run
	std::vector<RenderStageStats> Stages;
	// Per frame, not counting the UI's
	double DrawCalls = 0.0;
	double Instances = 0.0;
	// Whether the GPU counts pipeline statistics (Rendering::RenderThreadStats::HasGPUStatistics); zero without them
	bool HasGPUStatistics = false;
	// Per frame, the render thread's average over its own stats window. Unlike DrawCalls, this has the UI's: the GPU
	// counts everything a stage runs, and ImGuiRenderStage runs the UI.
	Rendering::GPUPipelineStatistics GPUStatistics;
	// Per frame: the probe captures and relights and the shadow maps that scene changes caused
	RenderWorkCounts Work;
};

// One frame of what RenderStats averages, for measuring frame by frame (FrameTimeCapture)
struct RenderFrameCounts
{
	// Recording every view's render stages
	double RecordTimeMS = 0.0;
	// Not counting the UI's
	U32 DrawCalls = 0;
	U32 Instances = 0;
};

class RenderEngine
{
public:
	RenderEngine();
	~RenderEngine();
	RenderEngine(const RenderEngine&) = delete;
	RenderEngine& operator=(const RenderEngine&) = delete;

public:
	// windowHandle is the window's SDL_Window (Window::GetNativeHandle)
	// deviceOptions: how the device is made (IGFXDevice::Initialize)
	void Initialize(void* windowHandle, const Rendering::GFXDeviceOptions& deviceOptions);
	// Renders the main view as one frame
	void Render(const char* frameName);
	// Finish() does all the work that the main render phase needs including device present
	void Finish();
	// Hands what's been recorded (e.g. uploads) to the render thread without rendering or presenting a frame
	void Submit();
	void Shutdown();

	// Presents on every vertical blank, or at once (tearing where allowed)
	void SetVSync(bool enabled);
	// Blocks until the swap chain can queue another frame (see Rendering::Renderer::WaitForPresentSlot)
	void WaitForPresentSlot();

	// Destroys all render stages, releasing any resources they hold, and what they keep for the main scene. Must be
	// called before ResourceHandler::ShutDown().
	void ReleaseRenderStages();

public:
	template <typename TRenderStageType, typename... Args>
	void AddRenderStage(Args... args);

	// The scene the main view shows: the active GameScene's, which the engine sets
	void SetMainScene(RenderScene& scene) { m_mainScene = &scene; }
	RenderScene& GetMainScene();

	// Draws into the main scene, for tools that draw over the main view (e.g. the editor's grid and gizmos). Lines
	// from a scene's own objects go into that scene's RenderScene instead.
	void DrawLine(const Vector3D& start, const Vector3D& end, const Vector3D& colour = Vector3D(1.0f, 0.0f, 0.0f), float alpha = 1.0f, const DebugLineFade& fade = DebugLineFade())
	{
		GetMainScene().DrawLine(start, end, colour, alpha, fade);
	}
	void DrawWireBox(const BoundingBox& box, const Matrix4x4& transform, const Vector3D& colour, float alpha = 1.0f)
	{
		GetMainScene().DrawWireBox(box, transform, colour, alpha);
	}
	void DrawWireSphere(const Vector3D& centre, float radius, const Vector3D& colour, float alpha = 1.0f, float occludedAlpha = 0.0f)
	{
		GetMainScene().DrawWireSphere(centre, radius, colour, alpha, occludedAlpha);
	}

	void ResizeCanvas(const Vector2D& newSize);
	const Vector2D& GetCanvasSize() const;

	// The view presented to the user. Its owner (the app and the active camera) fills in its inputs.
	RenderView& GetMainView() { return m_mainView; }

	// The probes' budgets, for every scene; read by IrradianceProbeStage and ReflectionProbeStage each frame
	const GlobalIlluminationSettings& GetGlobalIlluminationSettings() const { return m_globalIlluminationSettings; }
	void SetGlobalIlluminationSettings(const GlobalIlluminationSettings& settings) { m_globalIlluminationSettings = settings; }

	// Renders scene into a secondary view (e.g. a camera preview or a thumbnail) as part of the current frame. The view
	// must have a Target.
	void RenderSecondaryView(const char* frameName, RenderView& view, RenderScene& scene);

	// Empty until the first window of frames has passed. Main thread only.
	const RenderStats& GetStats() const { return m_stats; }
	// The last frame that finished: the one before the frame being rendered. Main thread only.
	const RenderFrameCounts& GetLastFrame() const { return m_lastFrame; }

private:
	void InternalAddRenderStage(IRenderStage* stage);
	// Ends the frame so far as the next one starts rendering: it becomes the last frame, and is added to the stats window
	void EndFrameCounts();
	// Ends the stats window's frame as the next one starts rendering; the first frame only starts the window
	void UpdateStats(double now);

private:
	RenderScene* m_mainScene = nullptr;

	Vector2D m_canvasSize;

	RenderView m_mainView { ERenderViewKind::Main };
	std::unique_ptr<RenderPipeline> m_pipeline;
	// Advanced after the main view renders; secondary views rendered before it in the frame share its index
	U64 m_frameIndex = 0;
	GlobalIlluminationSettings m_globalIlluminationSettings;

	RenderStats m_stats;
	RenderFrameCounts m_lastFrame;
	// Seconds recording the frame so far: the main view, and secondary views rendered since the last main view
	double m_frameRecordTime = 0.0;
	StatsWindow m_statsWindow;
	// The stats window so far
	double m_windowRecordTime = 0.0;
	Rendering::DrawCounts m_windowDraws;
};

template <typename TRenderStageType, typename... Args>
void RenderEngine::AddRenderStage(Args... args)
{
	InternalAddRenderStage(new TRenderStageType(std::forward<Args>(args)...));
}
