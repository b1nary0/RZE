#pragma once

#include <UI/Framework/EditorEvents.h>

#include <Graphics/ViewCapture.h>

#include <functional>

namespace Editor
{
	class EditorContext;

	// Keeps each scene's thumbnail (Thumbnails::Scenes) up to date: captures the scene's view each time it's saved,
	// and when one that has no thumbnail yet finishes loading. Owned by EditorContext.
	class SceneThumbnailCapturer
	{
	public:
		using DoneCallback = std::function<void(bool succeeded)>;

	public:
		explicit SceneThumbnailCapturer(EditorContext& context);
		~SceneThumbnailCapturer() = default;

		SceneThumbnailCapturer(const SceneThumbnailCapturer&) = delete;
		SceneThumbnailCapturer& operator=(const SceneThumbnailCapturer&) = delete;

	public:
		// Once per frame (EditorContext::Update); takes a capture that's waiting for a loaded scene to settle
		void Update();

		// Captures the open scene's thumbnail now, through its main camera, or the editor camera if it has none.
		// onDone, if given, runs once: when the file is written, or straight away with false if the scene has no file
		// or no camera to capture from.
		void CaptureNow(DoneCallback onDone = nullptr);

	private:
		EditorContext& m_context;
		ViewCapture m_capture;
		// Frames left before a just-loaded scene is captured; negative when none is waiting
		int m_framesUntilCapture = -1;

		EventSubscription m_sceneSaved;
		EventSubscription m_sceneLoaded;
		EventSubscription m_sceneUnloading;
	};
}
