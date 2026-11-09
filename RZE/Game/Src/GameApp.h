#pragma once

#include <EngineApp.h>

class ResourceHandle;

class GameApp : public RZE_Application
{
public:

	GameApp();
	virtual ~GameApp();

	void ParseArguments(char** arguments, int count) override;

	void Initialize() override;
	void Start() override;
	void UpdateUI() override;
	void ShutDown() override;

	void RegisterInputEvents(InputHandler& inputHandler) override;

	InputGate ProcessInput(const InputHandler& handler) override;

	void OnWindowResize(const Vector2D& newSize) override;

protected:
	void CreateRenderTarget(const Vector2D& dimensions) override;

private:
	// The scene to start in, from -scene (the Editor's Run passes it); without one, Initialize falls back to a fixed
	// scene. A shipping game needs a start scene too, which Game.ini could own instead.
	std::string m_startScene;
};