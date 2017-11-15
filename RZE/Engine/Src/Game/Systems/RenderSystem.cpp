#include <StdAfx.h>
#include <Game/Systems/RenderSystem.h>

#include <Game/ECS/Entity.h>
#include <Game/ECS/EntityComponentFilter.h>

#include <Game/Components/LightSourceComponent.h>
#include <Game/Components/MeshComponent.h>
#include <Game/Components/TransformComponent.h>
#include <Game/Components/FontRenderComponent.h>

#include <RenderCore/Renderer.h>
#include <RenderCore/HardwareInterface/OpenGL.h>

#include <RenderCore/Graphics/Mesh.h>
#include <RenderCore/Graphics/Texture2D.h>

#include <RenderCore/Shaders/Shader.h>
#include <RenderCore/Shaders/ShaderGroup.h>

#include <Utils/Math/Vector4D.h>

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
	SetFilterTypes();
}

void RenderSystem::Update()
{
	RZE_Renderer* const renderer = mAdmin->GetRenderer();
	AssertNotNull(renderer);

	SceneCamera& renderCam = renderer->GetSceneCamera();
	renderCam.GenerateProjectionMat();
	renderCam.GenerateViewMat();

	std::vector<IEntity*> mainList;
	mRelevantComponents.FilterAnyOf(mAdmin->GetEntities(), mainList);

	//
	// Rendered
	//
	EntityComponentFilter renderPassFilter;
	renderPassFilter.AddFilterType<MeshComponent>();
	renderPassFilter.AddFilterType<TransformComponent>();

	std::vector<IEntity*> renderPassEntities;
	renderPassFilter.FilterAtLeast(mainList, renderPassEntities);

	for (auto& entity : renderPassEntities)
	{
		MeshComponent* const meshComponent = entity->GetComponent<MeshComponent>();
		TransformComponent* const transformComponent = entity->GetComponent<TransformComponent>();


		Matrix4x4 modelMat;
		modelMat.Translate(transformComponent->GetPosition());
		modelMat.Rotate(transformComponent->GetRotation().ToAngle(), transformComponent->GetRotation().ToAxis());
		modelMat.Scale(transformComponent->GetScale());

		RZE_Renderer::RenderItemProtocol renderItem;
		renderItem.mShaderGroup = meshComponent->GetShaderGroup();
		renderItem.mModelMat = modelMat;
		renderItem.mProjectionMat = renderCam.GetProjectionMat();
		renderItem.mViewMat = renderCam.GetViewMat();

		// #TODO find a better transfer point for the resource handler. Maybe pass in as an argument to constructor for renderer?
		renderItem.mMeshData = RZE_Engine::Get()->GetResourceHandler().GetResource<MeshResource>(meshComponent->GetMeshHandle());

		if (meshComponent->GetTextureHandle().IsValid())
		{
			renderItem.mTextureData = RZE_Engine::Get()->GetResourceHandler().GetResource<GFXTexture2D>(meshComponent->GetTextureHandle());
		}

		if (renderer)
		{
			renderer->AddRenderItem(renderItem);
		}
	}

	//
	// Lights
	//
	EntityComponentFilter lightPassCFilter;
	lightPassCFilter.AddFilterType<TransformComponent>();
	lightPassCFilter.AddFilterType<LightSourceComponent>();

	std::vector<IEntity*> lightEntities;
	lightPassCFilter.FilterAtLeast(mainList, lightEntities);
	for (auto& entity : lightEntities)
	{
		LightSourceComponent* lightComponent = entity->GetComponent<LightSourceComponent>();
		TransformComponent* transformComponent = entity->GetComponent<TransformComponent>();

		RZE_Renderer::LightItemProtocol lightItem;
		lightItem.mLightColor = lightComponent->GetColor();
		lightItem.mLightPos = transformComponent->GetPosition();
		lightItem.mLightStrength = lightComponent->GetStrength();

		renderer->AddLightItem(lightItem);
	}
}

void RenderSystem::ShutDown()
{
}

void RenderSystem::SetFilterTypes()
{
	mRelevantComponents.AddFilterType<MeshComponent>();
	mRelevantComponents.AddFilterType<TransformComponent>();
	mRelevantComponents.AddFilterType<LightSourceComponent>();
}
