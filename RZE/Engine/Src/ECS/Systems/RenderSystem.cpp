#include <StdAfx.h>

#include <ECS/Systems/RenderSystem.h>

#include <ECS/Components/LightSourceComponent.h>
#include <ECS/Components/MeshComponent.h>
#include <ECS/Components/TransformComponent.h>

#include <Apollo/ECS/EntityComponentFilter.h>

#include <Diotima/Graphics/Mesh.h>
#include <Diotima/Shaders/ShaderGroup.h>

#include <Utils/Platform/Timers/HiResTimer.h>

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

	RZE_Engine::Get()->GetResourceHandler().ReleaseResource(vertShaderHandle);
	RZE_Engine::Get()->GetResourceHandler().ReleaseResource(fragShaderHandle);
}

RenderSystem::RenderSystem()
{
}

void RenderSystem::Initialize()
{
	Apollo::ComponentHandler& componentHandler = RZE_Engine::Get()->GetComponentHandler();

	Apollo::ComponentTypeID<Apollo::ComponentBase>::GetComponentTypeID<TransformComponent>();
	Apollo::ComponentTypeID<Apollo::ComponentBase>::GetComponentTypeID<MeshComponent>();

	InternalGetComponentFilter().AddFilterType<TransformComponent>();
	InternalGetComponentFilter().AddFilterType<MeshComponent>();

	Apollo::ComponentHandler::ComponentAddedFunc OnMeshComponentAdded([this](Apollo::EntityID entityID, Apollo::ComponentHandler& handler)
	{
		MeshComponent* const meshComp = handler.GetComponent<MeshComponent>(entityID);
		meshComp->Resource = RZE_Engine::Get()->GetResourceHandler().RequestResource<Diotima::MeshResource>(meshComp->ResourcePath);
	});

	componentHandler.RegisterForComponentAddNotification<MeshComponent>(OnMeshComponentAdded);

	CreateDefaultShader();
}

void RenderSystem::Update(std::vector<Apollo::EntityID>& entities)
{
 	Apollo::ComponentHandler& handler = RZE_Engine::Get()->GetComponentHandler();
  	Diotima::RenderSystem* const renderSystem = RZE_Engine::Get()->GetRenderSystem();
 	
	for (auto& entity : entities)
 	{
  		MeshComponent* const meshComp = handler.GetComponent<MeshComponent>(entity);
  		TransformComponent* const transfComp = handler.GetComponent<TransformComponent>(entity);
  
  		Diotima::RenderSystem::RenderItemProtocol item;
  
  		Matrix4x4 modelMat;
  		modelMat.Translate(transfComp->Position);
  		modelMat.Rotate(transfComp->Rotation.ToAngle(), transfComp->Rotation.ToAxis());
  		modelMat.Scale(transfComp->Scale);
  
  		item.MeshData = RZE_Engine::Get()->GetResourceHandler().GetResource<Diotima::MeshResource>(meshComp->Resource);
  		item.ModelMat = modelMat;
  		item.ProjectionMat = renderSystem->GetSceneCamera().GetProjectionMat();
  		item.ViewMat = renderSystem->GetSceneCamera().GetViewMat();
  		item.ShaderGroup = defaultShader;
  
  		renderSystem->AddRenderItem(item);
 	}

	Functor<void, Apollo::EntityID> LightSourceFunc([this, &handler, &renderSystem](Apollo::EntityID entity)
	{
		LightSourceComponent* const lightComp = handler.GetComponent<LightSourceComponent>(entity);
		TransformComponent* const transfComp = handler.GetComponent<TransformComponent>(entity);

		Diotima::RenderSystem::LightItemProtocol item;
		item.LightColor = lightComp->Color;
		item.LightStrength = lightComp->Strength;
		item.LightPos = transfComp->Position;

		renderSystem->AddLightItem(item);
	});

	handler.ForEach<LightSourceComponent, TransformComponent>(LightSourceFunc);
}

void RenderSystem::ShutDown()
{
}
