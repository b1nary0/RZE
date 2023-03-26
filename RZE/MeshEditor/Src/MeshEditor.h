#pragma once

#include <EngineApp.h>

#include <Game/World/GameObject/GameObject.h>

namespace MeshEditor
{
	class MeshEditorCameraComponent;

	class MeshEditorApp : public RZE_Application
	{
	public:
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
		GameObjectPtr m_editorCamera;
	};
}