#pragma once

#include "Application.h"

class GameApp : public RZE_Game
{
public:

	GameApp();
	virtual ~GameApp();

	virtual void RegisterEvents(EventHandler& eventHandler) override;
};