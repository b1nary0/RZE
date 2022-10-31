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
	void Update() override;
	void ShutDown() override;

	void RegisterInputEvents(InputHandler& inputHandler) override;

	bool ProcessInput(const InputHandler& handler) override;

	void OnWindowResize(const Vector2D& newSize) override;

protected:
	void CreateRenderTarget(const Vector2D& dimensions) override;

private:
	// #TODO Look into hiding this behind a debug ifdef or something?
	// should really only be used in dev builds
	std::string m_startScene;
};