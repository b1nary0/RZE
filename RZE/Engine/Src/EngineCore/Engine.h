#pragma once

#include <RZE_Config.h>

#include <Utils/Platform/File.h>

// @todo:josh this is where the weird include-before issue is
#include <Events/EventHandler.h>

#include <EngineCore/Config/EngineConfig.h>
#include <EngineCore/Input/InputHandler.h>
#include <EngineCore/Resources/ResourceHandler.h>

#include <Game/GameScene.h>

class RZE_Application;

class RenderEngine;
class Win32Window;

struct FilePromptParams;

class RZE_Engine
{
public:
	RZE_Engine();
	~RZE_Engine();

	bool IsInitialized() { return m_isInitialized; }

	void Run(Functor<RZE_Application* const>& createApplicationCallback);

	[[nodiscard]]
	const Vector2D& GetWindowSize() const;

	// #TODO(Josh) Need to figure out this weird connection of accessing engine from application, and application from engine.
	RZE_Application&			GetApplication();
	// #TODO
	// All of these gets might be best to hide behind a cpp. Or static engine in CPP memory with accessor in h.
	ResourceHandler&			GetResourceHandler();
	InputHandler&				GetInputHandler();
	RenderEngine&			GetRenderEngine();
	GameScene&					GetActiveScene();

	// #TODO(Should probably put the window in the app..)
	bool ShowOpenFilePrompt(const FilePromptParams& params, std::string& chosenPath);

	[[nodiscard]]
	double GetDeltaTime() const { return m_deltaTime; }
	[[nodiscard]]
	float GetDeltaTimeMS() const { return static_cast<float>(m_deltaTime * 1000); }
	
	void PostExit();

public:
	void Log(const std::string& text, const Vector3D& color);

	void SetWindowSize(Vector2D newSize);

private:

	void Init();
	void PostInit(Functor<RZE_Application* const>& createApplicationCallback);

	void PreUpdate(); // Set up anything the new frame will need
	void Update();

	void BeginShutDown();
	void InternalShutDown();
	
	void CompileEvents();

	void RegisterWindowEvents();
	void RegisterKeyEvents();

	void LoadEngineConfig();
	void CreateAndInitializeWindow();
	void CreateAndInitializeRenderer();

	void InitializeApplication(Functor<RZE_Application* const> createGameCallback);

	float CalculateAverageFrametime();

private:
	RZE_Application* m_application;
	GameScene* m_activeScene;

	Win32Window* m_window;

	ResourceHandler m_resourceHandler;
	EventHandler m_eventHandler;
	InputHandler m_inputHandler;

	std::unique_ptr<RenderEngine> m_renderEngine;

	EngineConfig* m_engineConfig;
	
	// PODs
private:
	double m_deltaTime;
	U64 m_frameCount;

	bool m_isInitialized;
	bool m_shouldExit;

	std::vector<float> m_frameSamples;
};
