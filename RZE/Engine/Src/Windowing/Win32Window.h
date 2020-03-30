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

	// Compile input messages from Win32 api
	void CompileInputMessages(InputHandler& inputHandler);

	// Compile all other window messages from Win32 api
	void CompileWindowMessages(EventHandler& eventHandler);

	void Show();

	const OSWindowHandleData& GetOSWindowHandleData() const;

	void RegisterEvents(EventHandler& eventHandler);

	void SetWindowSize(const Vector2D& newSize);

	void Maximize();

	FilePath ShowOpenFilePrompt();

protected:

private:

	void Create(const WindowCreationParams& creationProtocol);

	void ProcessWinProcMessage(const WindowMessageAdaptor::WindowMessageInfo& messageInfo, EventHandler& eventHandler);

	void InternalSetWindowPosition(const Vector2D& pos);

	void InternalSetWindowSize(const Vector2D& size);


	OSWindowHandleData mOSWindowHandleData;
	WindowCreationParams mCreationData;

	std::string mTitle;
	Vector2D mDimensions;
	Vector2D mPos;

	bool bCursorEnabled;
};
