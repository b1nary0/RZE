#pragma once

#include <memory>

#include "Events/Events.h"

class EventHandler;
class Win32Window;

class RZE_Game
{

public:

	RZE_Game();
	~RZE_Game();

	virtual void Start();
	virtual void Update();

	bool IsRunning() const;

	virtual void RegisterEvents(EventHandler& eventHandler);

protected:

	virtual void Init();

	std::weak_ptr<Win32Window> mMainWindow;

	bool bIsRunning;
};
