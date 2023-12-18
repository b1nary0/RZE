#pragma once

// winapi
// @improvement does it suffice to have a pch here or refactor to avoid this in a .h?
#include <Windows.h>

// std lib
#include <memory>
#include <string>

// RZE
#include <Windowing/WindowMessageAdaptor.h>
#include <Utils/Math/Vector2D.h>

class EventHandler;
class InputHandler;

struct FilePromptParams
{
	std::string Name;
	Filepath DirectoryPath;
	std::string FiletypeFilter;
};

class Win32Window
{

private:

	typedef struct OSWindowHandleData
	{
		HWND windowHandle;
		HDC deviceContext;
		HGLRC renderContext;
		PIXELFORMATDESCRIPTOR pixelFormatDesc;
	} OSWindowHandleData;

public:

	struct WindowCreationParams
	{
		std::string		windowTitle;
		int				width{ -1 };
		int				height{ -1 };
	};

	Win32Window(const WindowCreationParams& creationProtocol);

	void SetCursorEnabled(bool enabled);
	void ResetCursorToCenter();

	void SetTitle(const std::string& title);

	const std::string& GetTitle() const;
	const Vector2D& GetDimensions() const;
	const Vector2D& GetClientSize() const;

	// Compile input messages from Win32 api
	void CompileInputMessages(InputHandler& inputHandler);

	// Compile all other window messages from Win32 api
	void CompileWindowMessages(EventHandler& eventHandler);

	void Show();

	const OSWindowHandleData& GetOSWindowHandleData() const;

	void RegisterEvents(EventHandler& eventHandler);

	void SetWindowSize(const Vector2D& newSize);

	void Maximize();

	bool ShowOpenFilePrompt(const FilePromptParams& params, std::string& chosenPath);

	bool ShowSaveFilePrompt(const FilePromptParams& params, std::string& chosenPath);

protected:

private:

	void Create(const WindowCreationParams& creationProtocol);

	void ProcessWinProcMessage(const WindowMessageAdaptor::WindowMessageInfo& messageInfo, EventHandler& eventHandler);

	void InternalSetWindowPosition(const Vector2D& pos);

	void InternalSetWindowSize(const Vector2D& size);

	// @note right now (and maybe forever) we need to query winapi to update our client size every time
	// a change in window happens
	void QueryClientSize();


	OSWindowHandleData m_osWindowHandleData;

	std::string m_title;
	Vector2D m_dimensions;
	Vector2D m_clientSize;
	Vector2D m_pos;

	bool bCursorEnabled;
};
