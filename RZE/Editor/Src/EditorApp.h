#pragma once

#include <EngineApp.h>

#include <UI/Panels/LogPanel.h>
#include <UI/Panels/ScenePanel.h>
#include <UI/Panels/SceneViewPanel.h>
#include <UI/Panels/ResourceMonitorPanel.h>
#include <UI/Modals/AsyncOperationModal.h>

#include <Utils/Functor.h>

struct ImFont;

class AsyncOperation;

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

		GameObjectPtr GetCameraObject() const { return m_editorCameraObject; }

	private:
		void DisplayMenuBar();
		void HandleGeneralContextMenu();
		void ResolvePanelState();

		void LoadFonts();
		void StyleSetup();

		void AddFilePathToWindowTitle(const std::string& path);

		// Runs AssetCpy.bat on the calling thread, forwarding each line of its output. Returns the process exit code.
		static int RunAssetCpy(const Functor<void, const std::string&>& onOutputLine);

		// Builds the game on a worker thread, optionally launching it once the build succeeds.
		void BuildGame(bool launchAfterBuild);
		bool IsBuildRunning() const;

	private:
		// Asynchronous; the scene is unloaded immediately and the new one streams in over the following frames.
		// An invalid filepath loads the default (new) scene.
		void LoadScene(const Filepath& filepath);
		bool IsSceneLoading() const;

	private:
		PanelStates m_panelStates;

		LogPanel m_logPanel;
		ScenePanel m_scenePanel;
		SceneViewPanel m_sceneViewPanel;
		ResourceMonitorPanel m_resourceMonitor;

		AsyncOperationModal m_asyncOperationModal;
		std::shared_ptr<AsyncOperation> m_sceneLoadOperation;
		std::shared_ptr<AsyncOperation> m_buildOperation;

		std::unordered_map<std::string, ImFont*> m_fontMapping;

		GameObjectPtr m_editorCameraObject;

		Filepath m_imguiConfigFilepath;
	};
}