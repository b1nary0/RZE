#pragma once

#include <UI/Framework/EditorEvents.h>
#include <UI/Framework/EditorSelection.h>
#include <UI/Framework/EditorSettings.h>
#include <UI/Framework/PanelManager.h>
#include <UI/Framework/ScenePicker.h>

#include <Commands/CommandQueue.h>

#include <Services/GameBuilder.h>
#include <Services/GameObjectClipboard.h>
#include <Services/ProjectWatcher.h>
#include <Services/SceneDocument.h>
#include <Services/ThumbnailRenderer.h>
#include <Services/SelectionBoundsDisplay.h>
#include <Services/UndoHistory.h>

#include <UI/Modals/FileBrowserModal.h>
#include <UI/Modals/UnsavedChangesPrompt.h>
#include <UI/TaskTracker.h>

#include <Game/World/GameObject/GameObjectDefinitions.h>

#include <Utils/Math/Vector2D.h>

class EditorCameraComponent;

namespace Editor
{
	// The SceneView's state that the rest of the editor reacts to. Written by SceneViewPanel each frame it's displayed.
	struct SceneViewportState
	{
		// Editor input (camera, shortcuts) only goes to the scene while this is set; see EditorApp::ProcessInput.
		// Cleared at the start of each frame (EditorContext::Update), so it stays false while the SceneView is hidden.
		bool IsHovered = false;
		// In pixels; the editor camera's aspect ratio follows it
		Vector2D Size;
	};

	// Editor-wide state and services that panels share, so they don't reach into EditorApp or each other.
	// Owned by EditorApp and handed to each panel when it's displayed.
	class EditorContext
	{
	public:
		EditorContext();
		~EditorContext() = default;

		EditorContext(const EditorContext&) = delete;
		EditorContext& operator=(const EditorContext&) = delete;

	public:
		// Once per frame, before panels update or draw. Before ImGui's frame starts, so no ImGui calls.
		void Update();

		// Drops everything that refers to the current scene's objects (selection, editor camera, a pick in flight,
		// pending commands, undo history), then publishes SceneUnloadingEvent so listeners drop theirs (e.g. each panel's
		// EditorPanel::OnSceneReset). Call before the scene unloads, so cleanup that touches those objects (e.g.
		// selection bounds) still can. UI state, like the SceneView's viewport, carries over.
		void ResetSceneState();

		EditorEvents& GetEvents() { return m_events; }

		EditorSettings& GetSettings() { return m_settings; }
		const EditorSettings& GetSettings() const { return m_settings; }

		EditorSelection& GetSelection() { return m_selection; }
		const EditorSelection& GetSelection() const { return m_selection; }

		// Null while a scene is loading; the camera is created once the scene is in
		GameObjectPtr GetEditorCamera() const { return m_editorCamera; }
		// Null whenever GetEditorCamera() is
		GameObjectComponentPtr<EditorCameraComponent> GetEditorCameraComponent() const;
		void SetEditorCamera(GameObjectPtr editorCamera) { m_editorCamera = editorCamera; }

		SceneViewportState& GetSceneViewport() { return m_sceneViewport; }
		const SceneViewportState& GetSceneViewport() const { return m_sceneViewport; }

		ScenePicker& GetScenePicker() { return m_scenePicker; }

		PanelManager& GetPanels() { return m_panels; }

		SceneDocument& GetSceneDocument() { return m_sceneDocument; }
		GameBuilder& GetGameBuilder() { return m_gameBuilder; }
		GameObjectClipboard& GetClipboard() { return m_clipboard; }
		// Renders scene and mesh thumbnails in the background
		ThumbnailRenderer& GetThumbnails() { return m_thumbnails; }
		// Tells when the project's files change on disk (ProjectFilesChangedEvent)
		ProjectWatcher& GetProjectWatcher() { return m_projectWatcher; }

		// Where panels send scene changes; flushed once per frame after every panel has drawn
		CommandQueue& GetCommands() { return m_commands; }

		// What the commands did, for undo and redo; cleared when the scene unloads
		UndoHistory& GetUndoHistory() { return m_undoHistory; }
		const UndoHistory& GetUndoHistory() const { return m_undoHistory; }

		// For anything that would lose unsaved scene changes (New, Load, Exit): asks about saving them first
		UnsavedChangesPrompt& GetUnsavedChangesPrompt() { return m_unsavedChangesPrompt; }

		// Picks a file in a modal browser of thumbnail tiles
		FileBrowserModal& GetFileBrowser() { return m_fileBrowser; }

		// The background tasks shown, and the history of everything they did; for the status bar and the Task History
		// window. Updated once a UI frame, before anything draws (EditorApp::UpdateUI).
		TaskTracker& GetTasks() { return m_tasks; }
		const TaskTracker& GetTasks() const { return m_tasks; }

	private:
		// First, so it's destroyed last: everything below may hold subscriptions
		EditorEvents m_events;
		EditorSettings m_settings;
		EditorSelection m_selection;
		SelectionBoundsDisplay m_selectionBounds;
		GameObjectPtr m_editorCamera;
		SceneViewportState m_sceneViewport;
		ScenePicker m_scenePicker;
		PanelManager m_panels;
		SceneDocument m_sceneDocument;
		ThumbnailRenderer m_thumbnails;
		ProjectWatcher m_projectWatcher;
		GameBuilder m_gameBuilder;
		GameObjectClipboard m_clipboard;
		CommandQueue m_commands;
		UndoHistory m_undoHistory;
		UnsavedChangesPrompt m_unsavedChangesPrompt;
		FileBrowserModal m_fileBrowser;
		TaskTracker m_tasks;
		// Reload a thumbnail in the file browser once it's written again, and its files once they change. Last, so
		// they're dropped before the browser.
		EventSubscription m_thumbnailWritten;
		EventSubscription m_projectFilesChanged;
	};
}
