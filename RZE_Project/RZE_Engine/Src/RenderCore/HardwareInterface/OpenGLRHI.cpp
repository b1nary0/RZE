#include "StdAfx.h"

#include "OpenGLRHI.h"

OpenGLRHI::OpenGLRHI()
{
}

OpenGLRHI::~OpenGLRHI()
{
}

void OpenGLRHI::ClearColor(float r, float g, float b)
{
	glClearColor(r, g, b, 1.0f);
}

void OpenGLRHI::Clear(const GLbitfield bitfield)
{
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
}
