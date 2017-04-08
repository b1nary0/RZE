#include "Engine.h"

#include <cstdio>

RZE_EngineCore::RZE_EngineCore()
{
	Init();
}

RZE_EngineCore::~RZE_EngineCore()
{
}

std::weak_ptr<Win32Window> RZE_EngineCore::CreateWindow(const std::string& title, const int width, const int height)
{
	return mWindowManager.MakeWindow(title, width, height);
}

void RZE_EngineCore::Init()
{
	printf("RZE_EngineCore::Init() called. \n");
}
