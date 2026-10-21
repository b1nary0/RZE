#pragma once

#include <Services/ThumbnailQueue.h>
#include <UI/Framework/EditorEvents.h>

#include <Game/HeadlessScene.h>

#include <Graphics/ViewCapture.h>

#include <memory>

class AsyncOperation;
class Filepath;

namespace Editor
{
	class EditorContext;
	struct FileBrowserRequest;

	// Renders scene and mesh thumbnails (Thumbnails::Scenes, Thumbnails::Meshes) in the background, one at a time, in a
	// HeadlessScene of its own: the editor's scene is never touched, and nothing waits on it. What's asked for goes on a
	// ThumbnailQueue, which one "Rendering thumbnails" operation at a time works through (the status bar shows it and
	// can cancel it). Thumbnails that are up to date are skipped unless forced.
	//  - A scene is loaded on its own and captured through its main camera, or framed from its bounds if it has none.
	//  - A mesh is staged in Default.scene, framed from a three-quarter angle, and lit uniformly
	//    (ERenderViewLighting::Uniform) at a fixed exposure.
	// Each thumbnail written publishes a ThumbnailWrittenEvent, so browsers showing it reload it. A scene's is asked for when it's saved, forced,
	// and when it's loaded. Owned by EditorContext.
	class ThumbnailRenderer
	{
	public:
		explicit ThumbnailRenderer(EditorContext& context);
		~ThumbnailRenderer();

		ThumbnailRenderer(const ThumbnailRenderer&) = delete;
		ThumbnailRenderer& operator=(const ThumbnailRenderer&) = delete;

	public:
		// Queues asset's thumbnail if it's a scene or a mesh asset (anything else is ignored) and it isn't up to date, or
		// force is set, and starts rendering if it isn't already
		void Request(const Filepath& asset, bool force = false);

		// For a browser of scenes or meshes: queues the thumbnails its tiles are missing, and adds Refresh Thumbnail to
		// their menus. Leaves a browser of other files alone.
		void AddToBrowser(FileBrowserRequest& request);

		bool IsRendering() const { return m_job != nullptr; }

		// Cancels rendering and unloads its scene, while the renderer is still up (EditorApp::ShutDown)
		void ShutDown();

	private:
		void StartIfNeeded();

	private:
		EditorContext& m_context;
		ThumbnailQueue m_queue;
		HeadlessScene m_scene;
		ViewCapture m_capture;

		// While one's running or finishing; the next starts when it's done, if more were asked for meanwhile
		std::shared_ptr<AsyncOperation> m_job;
		bool m_isShutDown = false;

		EventSubscription m_sceneSaved;
		EventSubscription m_sceneLoaded;
	};
}
