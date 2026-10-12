#pragma once

#include <EngineApp.h>

#include <UI/MainMenuBar.h>
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
		void Update() override;
		void ShutDown() override;

		void ParseArguments(char** arguments, int count) override;
		void RegisterInputEvents(InputHandler& inputHandler) override;
		bool ProcessInput(const InputHandler& handler) override;

		void OnWindowResize(const Vector2D& newSize) override;

	protected:
		void CreateRenderTarget(const Vector2D& dimensions) override;

	private:
		void LoadFonts();
		void StyleSetup();

		void AddFilePathToWindowTitle(const std::string& path);

	private:
		EditorContext m_context;

		MainMenuBar m_mainMenuBar;
		AsyncOperationModal m_asyncOperationModal;

		std::unordered_map<std::string, ImFont*> m_fontMapping;

		Filepath m_imguiConfigFilepath;
	};
}