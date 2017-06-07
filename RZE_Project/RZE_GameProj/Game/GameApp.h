#pragma once

#include "Application.h"

class GameApp : public RZE_Game
{
public:

	GameApp();
	virtual ~GameApp();

	virtual void RegisterEvents(EventHandler& eventHandler) override;

	virtual void Start() override;
	virtual void Update() override;

private:
	void CreateTestShaderStuff(class MeshComponent* const meshComponent);
};