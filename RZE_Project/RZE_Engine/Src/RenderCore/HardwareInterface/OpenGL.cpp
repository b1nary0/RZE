#include "StdAfx.h"

#include "OpenGL.h"

OpenGLRHI::OpenGLRHI()
{
}

OpenGLRHI::~OpenGLRHI()
{
}

void OpenGLRHI::Init()
{
	// @implementation these need to be read in via window attributes
	glViewport(0, 0, 1280, 720);

	InitGLEW();
}

void OpenGLRHI::InitGLEW()
{
	GLenum err = glewInit();
	if (err == GLEW_OK)
	{
		LOG_CONSOLE("GLEW initialized successfully.");
	}
	else
	{
		// @todo do something meaningful here
		LOG_CONSOLE("GLEW did not initialize.");
		AssertFalse();
	}
}

void OpenGLRHI::ClearColor(const Float32 red, const Float32 green, const Float32 blue, const Float32 alpha) const
{
	glClearColor(red, green, blue, alpha);
}

void OpenGLRHI::Clear(const UInt32 mask) const
{
	glClear(mask);
}

void OpenGLRHI::GenVertexArrays(UInt32* outObjectID, const UInt32 arrayCount) const
{
	AssertExpr(arrayCount > 0);
	glGenVertexArrays(arrayCount, outObjectID);
}

void OpenGLRHI::BindVertexArray(const UInt32 arrayObjectHandle) const
{
	glBindVertexArray(arrayObjectHandle);
}

void OpenGLRHI::GenerateBuffer(UInt32* outBufferObjectHandle, UInt32 bufferCount) const
{
	AssertExpr(bufferCount > 0);
	glGenBuffers(bufferCount, outBufferObjectHandle);
}

void OpenGLRHI::BindBuffer(const UInt32 target, const UInt32 bufferObjectHandle) const
{
	glBindBuffer(target, bufferObjectHandle);
}

void OpenGLRHI::SetBufferData(const UInt32 target, const UInt32 size, const void* const data, const UInt32 bufferUsage) const
{
	AssertNotNull(data);
	AssertExpr(size > 0);
	// @note if this doesnt work check the const void* const signature because its possible under the hood it no likey
	//			
	glBufferData(target, size, data, bufferUsage);
}

void OpenGLRHI::EnableVertexAttributeArray(const UInt32 index) const
{
	glEnableVertexAttribArray(index);
}

void OpenGLRHI::VertexAttribPointer(const UInt32 index, const Int32 size, const UInt32 type, const bool normalized, const UInt32 stride, const void* const pointer)
{
	AssertExpr(size > 0);
	glVertexAttribPointer(index, size, type, normalized, stride, pointer);
}

void OpenGLRHI::DrawArrays(const UInt32 mode, const Int32 first, const UInt32 count)
{
	AssertExpr(count > 0);
	glDrawArrays(mode, first, count);
}
