#pragma once

#include <memory>

#include <Events/Events.h>

#include <Windowing/Win32Window.h>

class EventHandler;
class Win32Window;

class RZE_Application
{
public:
	RZE_Application() {}
	~RZE_Application() {}

	virtual void Start() {}
	virtual void Update() {}
	virtual void ShutDown() {}

	bool IsRunning() const { return bIsRunning; }

protected:
	void SetRunning(bool bRunning) { bIsRunning = bRunning; }

private:
	virtual void Init() {};

	bool bIsRunning;
};

class RZE_Game : public RZE_Application
{
	// @note maybe review this connection later
	friend class RZE_Engine;

public:
	RZE_Game();
	virtual ~RZE_Game();

	virtual void Start() override;
	virtual void Update() override;
	virtual void ShutDown() override;

	virtual void RegisterEvents(EventHandler& eventHandler);

protected:

	void ShowWindow();

	Win32Window* const GetWindow() const;

private:
	virtual void Init() override;

	void SetWindow(Win32Window* const window);
	void CompileEvents(EventHandler& eventHandler);

	Win32Window* mWindow;
};
