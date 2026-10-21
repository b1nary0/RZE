#pragma once

#include <EngineApp.h>

#include <UI/BottomDockArea.h>
#include <UI/MainMenuBar.h>
#include <UI/StatusBar.h>
#include <UI/Modals/AsyncOperationModal.h>
#include <UI/Framework/EditorContext.h>

struct ImFont;

namespace Editor
{
	class EditorApp : public RZE_Application
	{
	public:
		EditorApp();
		~EditorApp() override;

		void Initialize() override;
		void Start() override;
		void PreUIUpdate() override;
		void UpdateUI() override;
		void PreRender() override;
		void ShutDown() override;

		void ParseArguments(char** arguments, int count) override;
		void RegisterInputEvents(InputHandler& inputHandler) override;
		bool ProcessInput(const InputHandler& handler) override;

		void OnWindowResize(const Vector2D& newSize) override;
		// Asks about unsaved changes first (UnsavedChangesPrompt), which exits once they're dealt with
		bool OnCloseRequested() override;

	protected:
		void CreateRenderTarget(const Vector2D& dimensions) override;

	private:
		void LoadFonts();
		void StyleSetup();

		// "RZEStudio - <scene file>", with a * while it has unsaved changes. Drawn in the main menu bar, which is the title
		// bar, and shown on the taskbar.
		void RefreshWindowTitle(const SceneDocument& document);

	private:
		EditorContext m_context;

		MainMenuBar m_mainMenuBar;
		StatusBar m_statusBar;
		BottomDockArea m_bottomDockArea;
		AsyncOperationModal m_asyncOperationModal;

		std::unordered_map<std::string, ImFont*> m_fontMapping;

		Filepath m_imguiConfigFilepath;

		// Keep the window title on the scene's file and modified state
		std::vector<EventSubscription> m_titleSubscriptions;
	};
}