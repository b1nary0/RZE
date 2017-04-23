#include "StdAfx.h"

#include "RenderCore/Renderer.h"

#include "HardwareInterface/OpenGL.h"
#include "Windowing/Win32Window.h"

RZE_Renderer::RZE_Renderer()
{
	mOpenGLInterface = new OpenGLDriverInterface();
}

RZE_Renderer::~RZE_Renderer()
{
	AssertNotNull(mOpenGLInterface);
	delete mOpenGLInterface;
}

void RZE_Renderer::Render()
{
	mOpenGLInterface->Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void RZE_Renderer::Init()
{
	mOpenGLInterface->ClearColor(0.5f, 0.5f, 0.5f, 1.0f);
}
