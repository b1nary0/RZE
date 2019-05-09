#pragma once

#include <EngineApp.h>

class EditorApp : public RZE_Application
{
public:
	EditorApp();
	virtual ~EditorApp();

	virtual void Initialize() override;
	virtual void Start() override;
	virtual void Update() override;
	virtual void ShutDown() override;

	virtual void RegisterInputEvents(InputHandler& inputHandler) override;

private:
	std::bitset<32> mWindowBits; // Enable/Disable showing certain panels
};