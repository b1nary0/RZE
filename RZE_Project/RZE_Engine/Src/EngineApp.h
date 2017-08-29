#pragma once


#include <memory>

#include <Events/Events.h>

#include <Windowing/Win32Window.h>

class EventHandler;
class Win32Window;

class RZE_Game
{
	// @note maybe review this connection later
	friend class RZE_Engine;

public:

	RZE_Game();
	~RZE_Game();

	virtual void Start();
	virtual void Update();
	virtual void ShutDown();

	bool IsRunning() const;

	virtual void RegisterEvents(EventHandler& eventHandler);

protected:

	virtual void Init();

	void ShowWindow();

	bool bIsRunning;

	Win32Window* const GetWindow() const;

private:

	void SetWindow(Win32Window* const window);
	void CompileEvents(EventHandler& eventHandler);

	Win32Window* mWindow;
};
