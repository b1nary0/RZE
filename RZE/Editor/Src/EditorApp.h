#pragma once

#include <EngineApp.h>

#include <UI/ScenePanel.h>

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
		virtual ~EditorApp();

		virtual void Initialize() override;
		virtual void Start() override;
		virtual void Update() override;
		virtual void ShutDown() override;

		virtual void RegisterInputEvents(InputHandler& inputHandler) override;

	private:
		void DisplayMenuBar();
		void HandleGeneralContextMenu();
		void ResolvePanelState();

	private:
		PanelStates mPanelStates;

		ScenePanel mScenePanel;
	};
}