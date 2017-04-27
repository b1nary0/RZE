#include "StdAfx.h"

#include "Game/Systems/RenderSystem.h"

#include "RenderCore/HardwareInterface/OpenGL.h"

RenderSystem::RenderSystem()
{
	OpenGLRHI::Get().Init();

	OpenGLRHI::Get().ClearColor(0.5f, 0.5f, 0.5f, 1.0f);
}

RenderSystem::~RenderSystem()
{
}

void RenderSystem::Update()
{
	const OpenGLRHI& openGL = OpenGLRHI::Get();

	openGL.Clear(EGLBufferBit::Color | EGLBufferBit::Depth);

	float verts[9] = { -0.5f, -0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
		0.0f, 0.5f, 0.0f };

	GLuint vao;
	openGL.GenVertexArrays(1, &vao);
	openGL.BindVertexArray(vao);

	// @implementation should we have this type of assumption?
	OpenGLVBO vbo;
	vbo.SetBufferData(verts, sizeof(verts));

	openGL.EnableVertexAttributeArray(0);
	openGL.VertexAttribPointer(0, 3, EGLDataType::Float, EGLBooleanValue::False, 0, 0);

	openGL.DrawArrays(EGLDrawMode::Triangles, 0, 3);
}
