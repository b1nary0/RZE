#include <StdAfx.h>
#include <Systems/EntityRenderSystem.h>

#include <Apollo/ECS/Entity.h>
#include <Apollo/ECS/EntityComponentFilter.h>

#include <Components/LightSourceComponent.h>
#include <Components/MeshComponent.h>
#include <Components/TransformComponent.h>

#include <Diotima/RenderSystem.h>
#include <Diotima/Graphics/Mesh.h>
#include <Diotima/Graphics/Texture2D.h>
#include <Diotima/Shaders/Shader.h>
#include <Diotima/Shaders/ShaderGroup.h>

#include <Utils/Math/Vector4D.h>

EntityRenderSystem::EntityRenderSystem()
	: IEntitySystem()
{
}

EntityRenderSystem::~EntityRenderSystem()
{
}

void EntityRenderSystem::Initialize()
{
	SetFilterTypes();
}

void EntityRenderSystem::Update(IEntityAdmin::EntityList& entities)
{
	Diotima::RenderSystem* const renderer = RZE_Engine::Get()->GetRenderSystem();
	AssertNotNull(renderer);

	renderer->ClearLists();

	Diotima::SceneCamera& renderCam = renderer->GetSceneCamera();
	renderCam.GenerateProjectionMat();
	renderCam.GenerateViewMat();

	std::vector<IEntity*> mainList;
	mRelevantComponents.FilterAnyOf(entities, mainList);

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

		Diotima::RenderSystem::RenderItemProtocol renderItem;
		renderItem.mShaderGroup = meshComponent->GetShaderGroup();
		renderItem.mModelMat = modelMat;
		renderItem.mProjectionMat = renderCam.GetProjectionMat();
		renderItem.mViewMat = renderCam.GetViewMat();

		// #TODO find a better transfer point for the resource handler. Maybe pass in as an argument to constructor for renderer?
		renderItem.mMeshData = RZE_Engine::Get()->GetResourceHandler().GetResource<Diotima::MeshResource>(meshComponent->GetMeshHandle());

		if (meshComponent->GetTextureHandle().IsValid())
		{
			renderItem.mTextureData = RZE_Engine::Get()->GetResourceHandler().GetResource<Diotima::GFXTexture2D>(meshComponent->GetTextureHandle());
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

		Diotima::RenderSystem::LightItemProtocol lightItem;
		lightItem.mLightColor = lightComponent->GetColor();
		lightItem.mLightPos = transformComponent->GetPosition();
		lightItem.mLightStrength = lightComponent->GetStrength();

		renderer->AddLightItem(lightItem);
	}
}

void EntityRenderSystem::ShutDown()
{
}

void EntityRenderSystem::SetFilterTypes()
{
	mRelevantComponents.AddFilterType<MeshComponent>();
	mRelevantComponents.AddFilterType<TransformComponent>();
	mRelevantComponents.AddFilterType<LightSourceComponent>();
}
