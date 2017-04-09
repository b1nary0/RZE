#pragma once

#include <memory>

#include "EngineCore/Events/Events.h"

class EventHandler;

class RZE_Application
{

public:

	RZE_Application();
	~RZE_Application();

	void Start();
	void Update();

	bool IsRunning() const;

	void RegisterEvents(EventHandler& eventHandler);

protected:

	void Init();


	bool bIsRunning;
};
