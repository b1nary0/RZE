#include <StdAfx.h>

#include <ECS/Systems/RenderSystem.h>

#include <ECS/Components/CameraComponent.h>
#include <ECS/Components/LightSourceComponent.h>
#include <ECS/Components/MeshComponent.h>
#include <ECS/Components/TransformComponent.h>
#include <ECS/Components/MaterialComponent.h>

#include <Apollo/ECS/EntityComponentFilter.h>

#include <Diotima/Graphics/Mesh.h>
#include <Diotima/Graphics/Texture2D.h>
#include <Diotima/Shaders/ShaderGroup.h>

#include <Utils/Platform/FilePath.h>
#include <Utils/Platform/Timers/HiResTimer.h>

Diotima::GFXShaderGroup* defaultShader;
Diotima::GFXShaderGroup* textureShader;
void CreateDefaultShader()
{
	const std::string vertShaderFilePath = FilePath("Engine/Assets/Shaders/VertexShader.shader").GetAbsolutePath();
	const std::string fragShaderFilePath = FilePath("Engine/Assets/Shaders/FragmentShader.shader").GetAbsolutePath();

	ResourceHandle vertShaderHandle = RZE_Engine::Get()->GetResourceHandler().RequestResource<Diotima::GFXShader>(vertShaderFilePath.c_str(), EGLShaderType::Vertex, "DefaultVertexShader");
	ResourceHandle fragShaderHandle = RZE_Engine::Get()->GetResourceHandler().RequestResource<Diotima::GFXShader>(fragShaderFilePath.c_str(), EGLShaderType::Fragment, "DefaultFragShader");

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
}

void CreateTextureShader()
{
	const std::string vertShaderFilePath = FilePath("Engine/Assets/Shaders/TextureVert.shader").GetAbsolutePath();
	const std::string fragShaderFilePath = FilePath("Engine/Assets/Shaders/TextureFrag.shader").GetAbsolutePath();

	ResourceHandle vertShaderHandle = RZE_Engine::Get()->GetResourceHandler().RequestResource<Diotima::GFXShader>(vertShaderFilePath.c_str(), EGLShaderType::Vertex, "TextureVertShader");
	ResourceHandle fragShaderHandle = RZE_Engine::Get()->GetResourceHandler().RequestResource<Diotima::GFXShader>(fragShaderFilePath.c_str(), EGLShaderType::Fragment, "TextureFragShader");

	Diotima::GFXShader* vertShader = RZE_Engine::Get()->GetResourceHandler().GetResource<Diotima::GFXShader>(vertShaderHandle);
	vertShader->Create();
	vertShader->Compile();

	Diotima::GFXShader* fragShader = RZE_Engine::Get()->GetResourceHandler().GetResource<Diotima::GFXShader>(fragShaderHandle);
	fragShader->Create();
	fragShader->Compile();

	textureShader = new Diotima::GFXShaderGroup("TextureShader");
	textureShader->AddShader(Diotima::GFXShaderGroup::EShaderIndex::Vertex, vertShader);
	textureShader->AddShader(Diotima::GFXShaderGroup::EShaderIndex::Fragment, fragShader);

	textureShader->AddUniform("UModelMat");
	textureShader->AddUniform("UProjectionMat");
	textureShader->AddUniform("UViewMat");

	textureShader->AddUniform("ULightPosition");
	textureShader->AddUniform("UViewPosition");
	textureShader->AddUniform("ULightColor");
	textureShader->AddUniform("ULightStrength");

	textureShader->AddUniform("UFragColor");
	//textureShader->AddUniform("UTexture2D");

	textureShader->GenerateShaderProgram();
}

RenderSystem::RenderSystem()
{
}

void RenderSystem::Initialize()
{
	Apollo::ComponentHandler& componentHandler = RZE_Engine::Get()->GetComponentHandler();

	Apollo::ComponentTypeID<Apollo::ComponentBase>::GetComponentTypeID<TransformComponent>();
	Apollo::ComponentTypeID<Apollo::ComponentBase>::GetComponentTypeID<MeshComponent>();
	Apollo::ComponentTypeID<Apollo::ComponentBase>::GetComponentTypeID<MaterialComponent>();

	InternalGetComponentFilter().AddFilterType<TransformComponent>();
	InternalGetComponentFilter().AddFilterType<MeshComponent>();

	RegisterForComponentNotifications();

	CreateDefaultShader();
	CreateTextureShader();
}

void RenderSystem::Update(std::vector<Apollo::EntityID>& entities)
{
	AssertNotNull(mMainCamera);

	Apollo::ComponentHandler& handler = RZE_Engine::Get()->GetComponentHandler();
	Diotima::Renderer* const renderSystem = RZE_Engine::Get()->GetRenderSystem();

	GenerateCameraMatrices();

	for (auto& entity : entities)
	{
		MeshComponent* const meshComp = handler.GetComponent<MeshComponent>(entity);
		TransformComponent* const transfComp = handler.GetComponent<TransformComponent>(entity);
		MaterialComponent* const matComp = handler.GetComponent<MaterialComponent>(entity);

		Diotima::Renderer::RenderItemProtocol item;

		Matrix4x4 modelMat;
		modelMat.Translate(transfComp->Position);
		modelMat.Rotate(transfComp->Rotation.ToAngle(), transfComp->Rotation.ToAxis());
		modelMat.Scale(transfComp->Scale);

		item.MeshData = RZE_Engine::Get()->GetResourceHandler().GetResource<Diotima::MeshResource>(meshComp->Resource);
		item.ModelMat = modelMat;
		item.ProjectionMat = mMainCamera->ProjectionMat;
		item.ViewMat = mMainCamera->ViewMat;
		item.ShaderGroup = matComp->ShaderGroup;

		if (matComp->Texture.IsValid())
		{
			item.TextureData = RZE_Engine::Get()->GetResourceHandler().GetResource<Diotima::GFXTexture2D>(matComp->Texture);
		}

		renderSystem->AddRenderItem(item);
	}

	Functor<void, Apollo::EntityID> LightSourceFunc([this, &handler, &renderSystem](Apollo::EntityID entity)
	{
		LightSourceComponent* const lightComp = handler.GetComponent<LightSourceComponent>(entity);
		TransformComponent* const transfComp = handler.GetComponent<TransformComponent>(entity);

		Diotima::Renderer::LightItemProtocol item;
		item.LightColor = lightComp->Color;
		item.LightStrength = lightComp->Strength;
		item.LightPos = transfComp->Position;
		item.ViewPos = mMainCamera->Position;

		renderSystem->AddLightItem(item);
	});

	handler.ForEach<LightSourceComponent, TransformComponent>(LightSourceFunc);
}

void RenderSystem::ShutDown()
{
}

void RenderSystem::RegisterForComponentNotifications()
{
	Apollo::ComponentHandler& handler = RZE_Engine::Get()->GetComponentHandler();

	//
	// MeshComponent
	//
	Apollo::ComponentHandler::ComponentAddedFunc OnMeshComponentAdded([this](Apollo::EntityID entityID, Apollo::ComponentHandler& handler)
	{
		MeshComponent* const meshComp = handler.GetComponent<MeshComponent>(entityID);
		meshComp->Resource = RZE_Engine::Get()->GetResourceHandler().RequestResource<Diotima::MeshResource>(meshComp->ResourcePath);
	});
	handler.RegisterForComponentAddNotification<MeshComponent>(OnMeshComponentAdded);

	//
	// CameraComponent
	//
	Apollo::ComponentHandler::ComponentAddedFunc OnCameraComponentAdded([this](Apollo::EntityID entityID, Apollo::ComponentHandler& handler)
	{
		this->mMainCamera = handler.GetComponent<CameraComponent>(entityID);
	});
	handler.RegisterForComponentAddNotification<CameraComponent>(OnCameraComponentAdded);

	//
	// MaterialComponent
	//
	Apollo::ComponentHandler::ComponentAddedFunc OnMaterialComponentAdded([this](Apollo::EntityID entityID, Apollo::ComponentHandler& handler)
	{
		MaterialComponent* const matComp = handler.GetComponent<MaterialComponent>(entityID);
		matComp->Texture = RZE_Engine::Get()->GetResourceHandler().RequestResource<Diotima::GFXTexture2D>(matComp->ResourcePath);
		matComp->ShaderGroup = textureShader;
	});
	handler.RegisterForComponentAddNotification<MaterialComponent>(OnMaterialComponentAdded);
}

void RenderSystem::GenerateCameraMatrices()
{
	mMainCamera->ProjectionMat = Matrix4x4::CreatePerspectiveMatrix(mMainCamera->FOV, mMainCamera->AspectRatio, mMainCamera->NearCull, mMainCamera->FarCull);
	mMainCamera->ViewMat = Matrix4x4::CreateViewMatrix(mMainCamera->Position, mMainCamera->Position + mMainCamera->Forward, mMainCamera->UpDir);
}
