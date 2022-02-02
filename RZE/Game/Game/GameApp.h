#pragma once

#include <EngineApp.h>

class ResourceHandle;

class GameApp : public RZE_Application
{
public:

	GameApp();
	virtual ~GameApp();

	virtual void Initialize() override;
	virtual void Start() override;
	virtual void Update() override;
	virtual void ShutDown() override;

	virtual void RegisterInputEvents(InputHandler& inputHandler) override;

	virtual bool ProcessInput(const InputHandler& handler);

};