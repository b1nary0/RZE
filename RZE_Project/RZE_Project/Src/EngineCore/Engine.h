#pragma once

#include "Windowing/WindowManager.h"

class RZE_EngineCore
{

public:

	RZE_EngineCore();
	~RZE_EngineCore();

	std::weak_ptr<Win32Window> MakeWindow(const std::string& title, const int width, const int height);

private:

	void Init();

	WindowManager mWindowManager;

};
