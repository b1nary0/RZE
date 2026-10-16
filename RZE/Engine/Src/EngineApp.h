#pragma once

#include <RZE.h>

#include <Windowing/Win32Window.h>

#include <memory>

class EventHandler;
class Win32Window;

namespace Rendering
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
	// The UI's step before ImGui's frame starts (and so before UpdateUI): gather what the UI shows. No ImGui calls:
	// there's no ImGui frame or current window here, so a stray widget call fails immediately (a null window access,
	// which a debugger stops on), which keeps UI drawing out of it.
	virtual void PreUIUpdate();
	// Inside ImGui's frame: the application's UI, and applying what it changes (e.g. the editor's commands). Only UI
	// work goes here; the rest of a frame's work goes in Update.
	virtual void UpdateUI();
	// Each frame after the UI and before the scene's update: the application's own per-frame work. Outside ImGui's
	// frame, so no ImGui calls.
	virtual void Update();
	// Each frame after the scene's update, right before the frame is rendered: per-frame scene overlays such as debug
	// lines, which are cleared after every render. Outside ImGui's frame, so no ImGui calls.
	virtual void PreRender();
	virtual void ShutDown();

	// The user asked to close the window. True exits; false keeps running, e.g. to ask about unsaved changes first,
	// and the application exits later itself (RZE_Engine::PostExit) if it should.
	virtual bool OnCloseRequested() { return true; }

	virtual void CreateRenderTarget(const Vector2D& dimensions) = 0;
	virtual void OnWindowResize(const Vector2D& newSize) = 0;

	// Returning TRUE will fire callbacks registered with RZE. Returning FALSE steals input from the engine.
	virtual bool ProcessInput(const InputHandler& handler);

	virtual void ParseArguments(char** arguments, int count);

	virtual void RegisterInputEvents(InputHandler& inputHandler);


	Rendering::RenderTargetTexture* GetRTT();

	// Registers the engine's component types: their names, factories and property descriptions. Initialize calls it;
	// code that uses components without running an application (e.g. unit tests) calls it itself. Once per process.
	static void RegisterEngineComponents();
	
protected:
	virtual void Initialize();

	Win32Window& InternalGetWindow();

	void SetRunning(bool bRunning);
	void ShowWindow();

	Win32Window* const GetWindow() const;

	std::unique_ptr<Rendering::RenderTargetTexture> m_renderTarget;

private:
	void SetWindow(Win32Window* const window);
	
	Win32Window* mWindow;
	bool bIsRunning;
};
