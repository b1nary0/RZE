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

void OpenGLRHI::GenVertexArrays(const UInt32 arrayCount, UInt32* outBufferHandle) const
{
	AssertExpr(arrayCount > 0);
	glGenVertexArrays(arrayCount, outBufferHandle);
}

void OpenGLRHI::BindVertexArray(const UInt32 arrayObjectHandle) const
{
	glBindVertexArray(arrayObjectHandle);
}

void OpenGLRHI::GenerateBuffer(UInt32 bufferCount, UInt32* outBufferHandle) const
{
	AssertExpr(bufferCount > 0);
	glGenBuffers(bufferCount, outBufferHandle);
}

void OpenGLRHI::BindBuffer(const UInt32 target, const UInt32 bufferObjectHandle) const
{
	glBindBuffer(target, bufferObjectHandle);
}

void OpenGLRHI::DeleteBuffer(UInt32 bufferCount, UInt32* bufferHandle)
{
	AssertExpr(bufferCount > 0 && bufferHandle);
	glDeleteBuffers(bufferCount, bufferHandle);
}

void OpenGLRHI::SetBufferData(const UInt32 target, const UInt32 size, const void* const data, const UInt32 bufferUsage) const
{
	AssertNotNull(data);
	AssertExpr(size > 0);
	// @note if this doesnt work check the const void* const signature because its possible under the hood it no likey
	glBufferData(target, size, data, bufferUsage);
}

void OpenGLRHI::EnableVertexAttributeArray(const UInt32 index) const
{
	glEnableVertexAttribArray(index);
}

void OpenGLRHI::VertexAttribPointer(const UInt32 index, const Int32 size, const UInt32 type, const bool normalized, const UInt32 stride, const void* const pointer) const
{
	AssertExpr(size > 0);
	glVertexAttribPointer(index, size, type, normalized, stride, pointer);
}

void OpenGLRHI::DrawArrays(const UInt32 mode, const Int32 first, const UInt32 count) const
{
	AssertExpr(count > 0);
	glDrawArrays(mode, first, count);
}



//
//	OPENGLVBO
//





OpenGLVBO::OpenGLVBO()
	: mBufferCount(0)
	, mBufferHandle(0)
	, mBufferTarget(EGLBufferTarget::ArrayBuffer)
	, mBufferUsageMode(EGLBufferUsage::StaticDraw)
{
	GenerateBuffers(1);
	BindBuffers(0);
}

// OpenGLVBO::OpenGLVBO(const UInt32 count)
// 	: mBufferHandle(0)
// 	, mBufferTarget(EGLBufferTarget::ArrayBuffer)
// 	, mBufferUsageMode(EGLBufferUsage::StaticDraw)
// {
// 	GenerateBuffers(count);
// 	AssertExpr(false && "Deal with multi buffers properly first.");
// 
// 	OpenGLRHI::Get().DeleteBuffer(mBufferCount, &mBufferHandle);
// }

OpenGLVBO::~OpenGLVBO()
{
}

void OpenGLVBO::ClearAndRegenBufferCount(const UInt32 newCount)
{
	OpenGLRHI::Get().DeleteBuffer(mBufferCount, &mBufferHandle);

	AssertExpr(newCount > 0);
	OpenGLRHI::Get().GenerateBuffer(newCount, &mBufferHandle);
}

void OpenGLVBO::SetBufferTarget(const UInt32 newBufferTarget)
{
	mBufferTarget = newBufferTarget;
}

void OpenGLVBO::SetBufferUsageMode(const UInt32 newBufferUsageMode)
{
	mBufferUsageMode = newBufferUsageMode;
}

void OpenGLVBO::SetBufferData(const void* const data, const UInt32 size)
{
	AssertNotNull(data);
	OpenGLRHI::Get().SetBufferData(mBufferTarget, size, data, mBufferUsageMode);
}

void OpenGLVBO::GenerateBuffers(const UInt32 count)
{
	AssertExpr(!mBufferHandle && count > 0);
	OpenGLRHI::Get().GenerateBuffer(count, &mBufferHandle);

	mBufferCount = count;
}

void OpenGLVBO::BindBuffers(const UInt32 index)
{
	OpenGLRHI::Get().BindBuffer(mBufferTarget, mBufferHandle);
}
