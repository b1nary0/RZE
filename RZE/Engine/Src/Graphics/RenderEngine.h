#pragma once

#include <Graphics/GraphicsDefines.h>
#include <Graphics/RenderScene.h>
#include <Graphics/RenderView.h>

#include <Rendering/BufferHandle.h>

#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector2D.h>
#include <Utils/Math/Vector3D.h>
#include <Utils/Math/Vector4D.h>

namespace Rendering
{
	class RenderTargetTexture;
}

class IRenderStage;
class RenderPipeline;

class RenderEngine
{
public:
	RenderEngine();
	~RenderEngine();
	RenderEngine(const RenderEngine&) = delete;
	RenderEngine(const RenderEngine&&) = delete;

public:
	void operator=(const RenderEngine&) = delete;
	void operator=(const RenderEngine&&) = delete;

public:
	void Initialize(void* windowHandle);
	// Renders the main view as one frame
	void Render(const char* frameName);
	// Finish() does all the work that the main render phase needs including device present
	void Finish();
	void Shutdown();

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
	void DrawWireBox(const Vector3D& boundsMin, const Vector3D& boundsMax, const Matrix4x4& transform, const Vector3D& colour, float alpha = 1.0f)
	{
		GetMainScene().DrawWireBox(boundsMin, boundsMax, transform, colour, alpha);
	}

	void ResizeCanvas(const Vector2D& newSize);
	const Vector2D& GetCanvasSize() const;

	// The view presented to the user. Its owner (the app and the active camera) fills in its inputs.
	RenderView& GetMainView() { return m_mainView; }

	// Renders scene into a secondary view (e.g. a camera preview or a thumbnail) as part of the current frame. The view
	// must have a Target.
	void RenderSecondaryView(const char* frameName, RenderView& view, RenderScene& scene);

private:
	void InternalAddRenderStage(IRenderStage* stage);

private:
	RenderScene* m_mainScene = nullptr;

	Vector2D m_canvasSize;

	RenderView m_mainView { ERenderViewKind::Main };
	std::unique_ptr<RenderPipeline> m_pipeline;
};

template <typename TRenderStageType, typename... Args>
void RenderEngine::AddRenderStage(Args... args)
{
	InternalAddRenderStage(new TRenderStageType(std::forward<Args>(args)...));
}
