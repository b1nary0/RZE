#pragma once

#include <memory>

#include "Events/Events.h"

class EventHandler;

class RZE_Application
{

public:

	RZE_Application();
	~RZE_Application();

	virtual void Start();
	virtual void Update();

	bool IsRunning() const;

	virtual void RegisterEvents(EventHandler& eventHandler);

protected:

	virtual void Init();

	bool bIsRunning;
};
