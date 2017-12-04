#include <StdAfx.h>

#include <ECS/Systems/RenderSystem.h>

#include <ECS/Components/MeshComponent.h>
#include <ECS/Components/TransformComponent.h>

#include <Apollo/ECS/EntityComponentFilter.h>

#include <Diotima/Graphics/Mesh.h>
#include <Diotima/Shaders/ShaderGroup.h>

#include <Utils/Platform/Timers/HiResTimer.h>

Apollo::EntityComponentFilter componentFilter;

Diotima::GFXShaderGroup* defaultShader;
 void CreateDefaultShader()
 {
 	const char* const vertShaderFilePath = "./../Engine/Assets/Shaders/VertexShader.shader";
 	const char* const fragShaderFilePath = "./../Engine/Assets/Shaders/FragmentShader.shader";
 
 	ResourceHandle vertShaderHandle = RZE_Engine::Get()->GetResourceHandler().RequestResource<Diotima::GFXShader>(vertShaderFilePath, EGLShaderType::Vertex, "DefaultVertexShader");
 	ResourceHandle fragShaderHandle = RZE_Engine::Get()->GetResourceHandler().RequestResource<Diotima::GFXShader>(fragShaderFilePath, EGLShaderType::Fragment, "DefaultFragShader");
 
	Diotima::GFXShader* vertShader = RZE_Engine::Get()->GetResourceHandler().GetResource<Diotima::GFXShader>(vertShaderHandle);
 	vertShader->Create();
 	vertShader->Compile();
 
	Diotima::GFXShader* fragShader = RZE_Engine::Get()->GetResourceHandler().GetResource<Diotima::GFXShader>(fragShaderHandle);
 	fragShader->Create();
 	fragShader->Compile();
 
 	defaultShader = new Diotima::GFXShaderGroup("DefaultShader");
 	defaultShader->AddShader(Diotima::GFXShaderGroup::EShaderIndex::Vertex, vertShader);
 	defaultShader->AddShader(Diotima::GFXShaderGroup::EShaderIndex::Fragment, fragShader);
 
 	defaultShader->AddUniform("UModelMat");
 	defaultShader->AddUniform("UProjectionMat");
 	defaultShader->AddUniform("UViewMat");
 
 	defaultShader->AddUniform("ULightPosition");
 	defaultShader->AddUniform("UViewPosition");
 	defaultShader->AddUniform("ULightColor");
 	defaultShader->AddUniform("ULightStrength");
 
	defaultShader->AddUniform("UFragColor");
 
	defaultShader->GenerateShaderProgram();
 
 	//RZE_Engine::Get()->GetResourceHandler().ReleaseResource(vertShaderHandle);
 	//RZE_Engine::Get()->GetResourceHandler().ReleaseResource(fragShaderHandle);
 }

RenderSystem::RenderSystem()
{
}

void RenderSystem::Initialize()
{
	Apollo::ComponentHandler& componentHandler = RZE_Engine::Get()->GetComponentHandler();

	Apollo::ComponentTypeID<Apollo::ComponentBase>::GetComponentTypeID<TransformComponent>();
	Apollo::ComponentTypeID<Apollo::ComponentBase>::GetComponentTypeID<MeshComponent>();

	componentFilter.AddFilterType<TransformComponent>();
	componentFilter.AddFilterType<MeshComponent>();

	Apollo::ComponentHandler::ComponentAddedFunc OnMeshComponentAdded([this](Apollo::EntityID entityID, Apollo::ComponentHandler& handler)
	{
		MeshComponent* const meshComp = handler.GetComponent<MeshComponent>(entityID);
		meshComp->Resource = RZE_Engine::Get()->GetResourceHandler().RequestResource<Diotima::MeshResource>(meshComp->ResourcePath);
	});

	componentHandler.RegisterForComponentAddNotification<MeshComponent>(OnMeshComponentAdded);

	CreateDefaultShader();
}

void RenderSystem::Update(std::vector<Apollo::Entity>& entities)
{
	HiResTimer entityFilterTimer;

	entityFilterTimer.Start();
		Apollo::ComponentHandler::EntityList relevantEntities;
		componentFilter.FilterEachOf(entities, relevantEntities);
	entityFilterTimer.Stop();

	Apollo::ComponentHandler& handler = RZE_Engine::Get()->GetComponentHandler();
	for (auto& entity : relevantEntities)
	{
		MeshComponent* const meshComp = handler.GetComponent<MeshComponent>(entity.mEntityID);
		TransformComponent* const transfComp = handler.GetComponent<TransformComponent>(entity.mEntityID);

		Diotima::RenderSystem* const renderSystem = RZE_Engine::Get()->GetRenderSystem();
		Diotima::RenderSystem::RenderItemProtocol item;

		Matrix4x4 modelMat;
		modelMat.Translate(transfComp->Position);
		modelMat.Rotate(transfComp->Rotation.ToAngle(), transfComp->Rotation.ToAxis());
		modelMat.Scale(transfComp->Scale);

		item.mMeshData = RZE_Engine::Get()->GetResourceHandler().GetResource<Diotima::MeshResource>(meshComp->Resource);
		item.mModelMat = modelMat;
		item.mProjectionMat = renderSystem->GetSceneCamera().GetProjectionMat();
		item.mViewMat = renderSystem->GetSceneCamera().GetViewMat();
		item.mShaderGroup = defaultShader;

		renderSystem->AddRenderItem(item);
	}

	static bool bPrintTest = true;
	if (bPrintTest)
	{
		LOG_CONSOLE_ARGS("Entity Filter took %f ms with %i entities.", entityFilterTimer.GetElapsed<float>() * 1000.0f, static_cast<int>(entities.size()));
		bPrintTest = false;
	}
}

void RenderSystem::ShutDown()
{
}
