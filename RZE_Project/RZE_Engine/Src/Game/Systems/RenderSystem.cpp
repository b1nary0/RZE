#include "StdAfx.h"

#include "Game/Systems/RenderSystem.h"
#include "Game/ECS/Entity.h"
#include "Game/Components/RenderComponent.h"

#include "RenderCore/HardwareInterface/OpenGL.h"

RenderSystem::RenderSystem(IEntityAdmin* const admin)
	: IEntitySystem(admin)
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
	for (auto& entity : mAdmin->GetEntities())
	{
		const RenderComponent* component = static_cast<const RenderComponent* const>(entity->GetComponents()[0]);

		openGL.Clear(EGLBufferBit::Color | EGLBufferBit::Depth);

		GLuint vao;
		openGL.GenVertexArrays(1, &vao);
		openGL.BindVertexArray(vao);

		// @implementation should we have this type of assumption?
		OpenGLVBO vbo;
		vbo.SetBufferData(component->GetVertexList().data(), sizeof(component->GetVertexList().data()) * component->GetVertexList().size());

		openGL.EnableVertexAttributeArray(0);
		openGL.VertexAttribPointer(0, 3, EGLDataType::Float, EGLBooleanValue::False, 0, 0);

		openGL.DrawArrays(EGLDrawMode::Triangles, 0, 3);
	}
}
