#pragma once

#include <EngineApp.h>

#include <UI/Panels/LogPanel.h>
#include <UI/Panels/ScenePanel.h>
#include <UI/Panels/SceneViewPanel.h>

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

		void ParseArguments(const char* arguments, int count) override;
		void RegisterInputEvents(InputHandler& inputHandler) override;
		bool ProcessInput(const InputHandler& handler) override;

		void OnWindowResize(const Vector2D& newSize) override {}

	public:
		void SetFont(const char* fontName);

		void Log(const std::string& msg);

	private:
		void DisplayMenuBar();
		void HandleGeneralContextMenu();
		void ResolvePanelState();

		void LoadFonts();
		void StyleSetup();

		void AddFilePathToWindowTitle(const std::string& path);
		void RunAssetCpy();

	private:
		PanelStates mPanelStates;

		LogPanel mLogPanel;
		ScenePanel mScenePanel;
		SceneViewPanel mSceneViewPanel;

		std::unordered_map<std::string, ImFont*> mFontMapping;

		FilePath mImguiConfigFilePath;

		const float kMenuBarHeight = 24.f;
	};
}