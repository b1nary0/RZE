#include "StdAfx.h"

#include "RenderCore/Renderer.h"

#include "HardwareInterface/OpenGL.h"
#include "Windowing/Win32Window.h"

RZE_Renderer::RZE_Renderer()
{
}

RZE_Renderer::~RZE_Renderer()
{
}

void RZE_Renderer::Render()
{
	OpenGLRHI::Get().Clear(EGLBufferBit::Color | EGLBufferBit::Depth);

	float verts[9] = { -0.5f, -0.5f, 0.0f, 
						0.5f, -0.5f, 0.0f, 
						0.0f, 0.5f, 0.0f };

	GLuint vao;
	OpenGLRHI::Get().GenVertexArrays(&vao, 1);
	OpenGLRHI::Get().BindVertexArray(vao);

	GLuint buf;
	OpenGLRHI::Get().GenerateBuffer(&buf, 1);

	OpenGLRHI::Get().BindBuffer(EGLBufferTarget::ArrayBuffer, buf);
	OpenGLRHI::Get().SetBufferData(EGLBufferTarget::ArrayBuffer, sizeof(verts), verts, EGLBufferUsage::StaticDraw);
	OpenGLRHI::Get().EnableVertexAttributeArray(0);
	OpenGLRHI::Get().VertexAttribPointer(0, 3, EGLDataType::Float, EGLBooleanValue::False, 0, 0);

	OpenGLRHI::Get().DrawArrays(EGLDrawMode::Triangles, 0, 3);
}

void RZE_Renderer::Init()
{
	OpenGLRHI::Get().Init();

	OpenGLRHI::Get().ClearColor(0.5f, 0.5f, 0.5f, 1.0f);
}
