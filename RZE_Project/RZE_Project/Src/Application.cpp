#include "Application.h"

#include "EngineCore/Engine.h"

RZE_Application::RZE_Application()
{
	mEngineCore = std::make_unique<RZE_EngineCore>();

	Init();
}

RZE_Application::~RZE_Application()
{

}

void RZE_Application::Init()
{
	printf("RZE_Application::Init() called. \n");

	mWindow = mEngineCore->CreateWindow("RZE_Application", 1280, 720);
}
