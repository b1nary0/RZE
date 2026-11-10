#pragma once

#include <RZE.h>

#include <EngineCore/Input/InputCore.h>

#include <Windowing/Window.h>

#include <memory>

class EventHandler;
class Filepath;
class Window;

namespace Rendering
{
	class RenderTargetTexture;
}

class RZE_Application
{
public:
	RZE_Application();
	virtual ~RZE_Application();

public:
	// The engine calls it once its window exists, before Start: keeps the window, registers the engine's components,
	// then OnInitialize
	void Initialize(Window& window);

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

	// The image a splash screen shows while the engine starts up. Invalid, the default, for no splash screen.
	virtual Filepath GetSplashScreenImage() const;
	// The title the splash screen and the window start with. Empty, the default, for the engine config's WindowTitle.
	virtual std::string GetWindowTitle() const;
	// Whether start-up is done and the window can be shown, e.g. once the start scene has loaded. Until it is, the
	// engine ticks async operations with no frames, under the splash screen if there is one. True by default.
	virtual bool IsReadyToShow() const;

	virtual void CreateRenderTarget(const Vector2D& dimensions) = 0;
	virtual void OnWindowResize(const Vector2D& newSize) = 0;

	// Once a frame, before the bindings fire: which devices' input goes to the bindings and the state the app reads.
	// The default takes both.
	virtual InputGate ProcessInput(const InputHandler& handler);

	virtual void ParseArguments(char** arguments, int count);

	virtual void RegisterInputEvents(InputHandler& inputHandler);


	Rendering::RenderTargetTexture* GetRTT();

	// Registers the engine's component types: their names, factories and property descriptions. Initialize calls it;
	// code that uses components without running an application (e.g. unit tests) calls it itself. Once per process.
	static void RegisterEngineComponents();
	
protected:
	// The application's own start-up, once it has its window and the engine's components are registered
	virtual void OnInitialize();

	Window& InternalGetWindow();

	void SetRunning(bool bRunning);
	void ShowWindow();

	Window* const GetWindow() const;

	std::unique_ptr<Rendering::RenderTargetTexture> m_renderTarget;

private:
	// Null until Initialize
	Window* mWindow;
	bool bIsRunning;
};
