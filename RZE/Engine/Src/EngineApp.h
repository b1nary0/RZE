#pragma once

class EventHandler;
class Win32Window;

class RZE_Application
{
	// @note maybe review this connection later
	friend class RZE_Engine;

public:
	RZE_Application();
	virtual ~RZE_Application();

	virtual void Start();
	virtual void Update();
	virtual void ShutDown();
	
protected:

	void SetRunning(bool bRunning);
	void ShowWindow();

	Win32Window* const GetWindow() const;

private:
	virtual void Init();

	void SetWindow(Win32Window* const window);

	Win32Window* mWindow;
	bool bIsRunning;
};
