#pragma once

#include "Game/ECS/EntityAdmin.h"

class RZE_Renderer;

class GameWorld : public IEntityAdmin
{

public:
	GameWorld();
	virtual ~GameWorld();

	virtual RZE_Renderer* const GetRenderer() const override;

	virtual void Init() override;
	virtual void Update() override;
	virtual void ShutDown() override;

private:
	void InitSystems();

private:
	RZE_Renderer* mRenderer;
};
