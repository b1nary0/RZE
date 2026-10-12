#pragma once

#include <UI/Framework/EditorSelection.h>
#include <UI/Framework/PanelManager.h>
#include <UI/Framework/ScenePicker.h>

#include <Services/GameBuilder.h>
#include <Services/SceneDocument.h>

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
		// Once per frame, before panels are displayed
		void Update();

		// Drops everything that refers to the current scene's objects (selection, editor camera, a pick in flight,
		// and whatever each panel holds; see EditorPanel::OnSceneReset). Call before the scene unloads, so cleanup
		// that touches those objects (e.g. selection bounds) still can. UI state, like the SceneView's viewport,
		// carries over.
		void ResetSceneState();

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

	private:
		EditorSelection m_selection;
		GameObjectPtr m_editorCamera;
		SceneViewportState m_sceneViewport;
		ScenePicker m_scenePicker;
		PanelManager m_panels;
		SceneDocument m_sceneDocument;
		GameBuilder m_gameBuilder;
	};
}
