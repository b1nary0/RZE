#pragma once

#include "Application.h"

class GameApp : public RZE_Application
{
public:

	GameApp();
	virtual ~GameApp();

	virtual void RegisterEvents(EventHandler& eventHandler) override;
};