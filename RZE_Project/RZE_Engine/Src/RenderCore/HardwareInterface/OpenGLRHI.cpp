#include "StdAfx.h"

#include "OpenGLRHI.h"

OpenGLRHI::OpenGLRHI()
{
}

OpenGLRHI::~OpenGLRHI()
{
}

void OpenGLRHI::ClearColor(float r, float g, float b, float a)
{
	glClearColor(r, g, b, a);
}

void OpenGLRHI::Clear(const GLbitfield bitfield)
{
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
}
