#include <StdAfx.h>
#include <Game/Systems/RenderSystem.h>

#include <Game/ECS/Entity.h>
#include <Game/Components/MeshComponent.h>

#include <RenderCore/Renderer.h>
#include <RenderCore/HardwareInterface/OpenGL.h>

RenderSystem::RenderSystem(IEntityAdmin* const admin)
	: IEntitySystem(admin)
{
	OpenGLRHI::Get().Init();

	OpenGLRHI::Get().ClearColor(0.5f, 0.5f, 0.5f, 1.0f);
}

RenderSystem::~RenderSystem()
{
}

void RenderSystem::Init()
{
}

void RenderSystem::Update()
{
	RZE_Renderer* const renderer = mAdmin->GetRenderer();
	if (renderer)
	{
		for (auto& entity : mAdmin->GetEntities())
		{
			MeshComponent* const meshComponent = static_cast<MeshComponent* const>(entity->GetComponents()[0]);
			RZE_Renderer::RenderItemProtocol renderItem;
			renderItem.VertexList = &(meshComponent->GetVertexList());
			renderItem.ShaderGroup = meshComponent->GetShaderGroup();

			if (renderer)
			{
				renderer->AddRenderItem(renderItem);
			}
		}

		renderer->Render();
	}
}

void RenderSystem::ShutDown()
{
}
