#pragma once

#include <RZE.h>

#include <Windowing/Win32Window.h>

#include <memory>

class EventHandler;
class Win32Window;

namespace Diotima
{
	class RenderTargetTexture;
}

class RZE_Application
{
	// @note maybe review this connection later
	friend class RZE_Engine;

public:
	RZE_Application();
	virtual ~RZE_Application();

public:
	virtual void Run();
	virtual void Start();
	virtual void Update();
	virtual void ShutDown();

	// Returning TRUE will fire callbacks registered with RZE. Returning FALSE steals input.
	virtual bool ProcessInput(const InputHandler& handler);

	virtual void RegisterInputEvents(InputHandler& inputHandler);

	static RZE_Engine& RZE() { return mEngine; }

protected:
	virtual void Initialize();

	Win32Window& InternalGetWindow();

	void SetRunning(bool bRunning);
	void ShowWindow();

	Win32Window* const GetWindow() const;

private:
	void SetWindow(Win32Window* const window);

	std::unique_ptr<Diotima::RenderTargetTexture> mRenderTarget;

	Win32Window* mWindow;
	bool bIsRunning;

	static RZE_Engine mEngine;
};
