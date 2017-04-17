#pragma once

#include <memory>

#include "Events/Events.h"

class EventHandler;
class Win32Window;

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

	std::weak_ptr<Win32Window> mMainWindow;

	bool bIsRunning;
};
