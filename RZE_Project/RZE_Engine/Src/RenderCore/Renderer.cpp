#include "StdAfx.h"

#include "RenderCore/Renderer.h"

#include "HardwareInterface/OpenGLRHI.h"

#include "Windowing/Win32Window.h"

RZE_Renderer::RZE_Renderer()
{
	OpenGLRHI::Get().ClearColor(0.0f, 0.0f, 0.0f);
}

RZE_Renderer::~RZE_Renderer()
{
}

void RZE_Renderer::Render()
{
	OpenGLRHI::Get().Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	SwapBuffers(mMainWindow->GetOSWindowHandleData().deviceContext);
}

std::weak_ptr<Win32Window> RZE_Renderer::MakeWindow(const std::string& title, const int width, const int height)
{
	Win32Window::WindowCreationParams params;
	params.windowTitle = title;
	params.width = width;
	params.height = height;

	mMainWindow = std::make_shared<Win32Window>(params);
	assert(mMainWindow && "Window was not created for some reason.");

	return mMainWindow;
}

std::weak_ptr<Win32Window> RZE_Renderer::GetMainWindow() const
{
	return mMainWindow;
}
