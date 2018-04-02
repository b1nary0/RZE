#pragma once

#include <EngineApp.h>

#include <Apollo/EntityHandler.h>

#include <vector>

class ResourceHandle;

class GameApp : public RZE_Application
{
public:

	GameApp();
	virtual ~GameApp();

	virtual void Initialize() override;
	virtual void Start() override;
	virtual void Update() override;

	virtual void RegisterInputEvents(InputHandler& inputHandler) override;

private:
	std::vector<Apollo::EntityID> mNanosuits;
};