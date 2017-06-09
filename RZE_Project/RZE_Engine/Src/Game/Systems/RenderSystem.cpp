#include <StdAfx.h>
#include <Game/Systems/RenderSystem.h>

#include <Game/ECS/Entity.h>
#include <Game/Components/MeshComponent.h>

#include <RenderCore/Renderer.h>
#include <RenderCore/HardwareInterface/OpenGL.h>

#include <RenderCore/Shaders/Shader.h>
#include <RenderCore/Shaders/ShaderGroup.h>

RenderSystem::RenderSystem(IEntityAdmin* const admin)
	: IEntitySystem(admin)
{
	OpenGLRHI::Get().ClearColor(0.5f, 0.5f, 0.5f, 1.0f);
}

RenderSystem::~RenderSystem()
{
}

void RenderSystem::Init()
{
	CreateTestShaderStuff();
}

void RenderSystem::Update()
{
	RZE_Renderer* const renderer = mAdmin->GetRenderer();
	if (renderer)
	{
		for (auto& entity : mAdmin->GetEntities())
		{
			MeshComponent* const meshComponent = static_cast<MeshComponent* const>(entity->GetComponents()[0]);
			if (!meshComponent->GetShaderGroup())
			{
				meshComponent->SetShaderGroup(mShaderGroup);
			}

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

void RenderSystem::CreateTestShaderStuff()
{
	File vertShaderFile("./../RZE_Engine/Assets/Shaders/VertexShader.shader");
	File fragShaderFile("./../RZE_Engine/Assets/Shaders/FragmentShader.shader");

	AssertExpr(!vertShaderFile.Empty() && !fragShaderFile.Empty());
	GFXShader* vertShader = new GFXShader(EGLShaderType::Vertex, "TestVertexShader", vertShaderFile.Content());
	vertShader->Create();
	vertShader->Compile();
	GFXShader* fragShader = new GFXShader(EGLShaderType::Fragment, "TestFragShader", fragShaderFile.Content());
	fragShader->Create();
	fragShader->Compile();

	mShaderGroup = new GFXShaderGroup("TestShaderGroup");
	mShaderGroup->AddShader(EGFXShaderType::Vertex, vertShader);
	mShaderGroup->AddShader(EGFXShaderType::Fragment, fragShader);

	mShaderGroup->AddUniform("UModelViewProjection");
	mShaderGroup->AddUniform("UFragColor");

	mShaderGroup->GenerateShaderProgram();
}
