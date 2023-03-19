#pragma once

#include <EngineApp.h>

#include <UI/Panels/LogPanel.h>
#include <UI/Panels/ScenePanel.h>
#include <UI/Panels/SceneViewPanel.h>
#include <UI/Panels/ResourceMonitorPanel.h>

struct ImFont;

namespace Editor
{
	class EditorApp : public RZE_Application
	{
	private:
		struct PanelStates
		{
			bool bDemoPanelEnabled{ false };
			bool bScenePanelEnabled{ false };
		};

	public:
		EditorApp();
		~EditorApp() override;

		void Initialize() override;
		void Start() override;
		void Update() override;
		void ShutDown() override;

		void ParseArguments(char** arguments, int count) override;
		void RegisterInputEvents(InputHandler& inputHandler) override;
		bool ProcessInput(const InputHandler& handler) override;

		void OnWindowResize(const Vector2D& newSize) override;

		// @TODO Re-architect editor API to avoid having these calls here. Probably event-driven.
		// i.e Fire GameObjectSelectedEvent from ScenePanel and react to it in SceneViewPanel
		GameObjectPtr GetSelectedObjectFromScenePanel();
		void ResetSelectedObject();

	protected:
		void CreateRenderTarget(const Vector2D& dimensions) override;

	public:
		void SetFont(const char* fontName);

		void Log(const std::string& msg);

		void CreateAndInitializeEditorCamera();

	private:
		void DisplayMenuBar();
		void HandleGeneralContextMenu();
		void ResolvePanelState();

		void LoadFonts();
		void StyleSetup();

		void AddFilePathToWindowTitle(const std::string& path);
		void RunAssetCpy();

	private:
		void LoadScene(const Filepath& filepath);

	private:
		PanelStates m_panelStates;

		LogPanel m_logPanel;
		ScenePanel m_scenePanel;
		SceneViewPanel m_sceneViewPanel;
		ResourceMonitorPanel m_resourceMonitor;

		std::unordered_map<std::string, ImFont*> m_fontMapping;

		GameObjectPtr m_editorCameraObject;

		Filepath m_imguiConfigFilepath;

		const float k_menuBarHeight = 24.f;
	};
}