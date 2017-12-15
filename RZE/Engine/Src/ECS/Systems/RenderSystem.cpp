#include <StdAfx.h>

#include <ECS/Systems/RenderSystem.h>

#include <ECS/Components/CameraComponent.h>
#include <ECS/Components/LightSourceComponent.h>
#include <ECS/Components/MeshComponent.h>
#include <ECS/Components/TransformComponent.h>
#include <ECS/Components/MaterialComponent.h>

#include <Apollo/ECS/EntityComponentFilter.h>

#include <Diotima/Graphics/Material.h>
#include <Diotima/Graphics/Mesh.h>
#include <Diotima/Graphics/Texture2D.h>
#include <Diotima/Shaders/ShaderGroup.h>

#include <Utils/Platform/FilePath.h>
#include <Utils/Platform/Timers/HiResTimer.h>

static Vector4D sDefaultFragColor(1.0f, 0.25f, 0.25f, 1.0f);

Diotima::GFXShaderGroup* defaultShader;
Diotima::GFXShaderGroup* textureShader;
void CreateDefaultShader()
{
	const FilePath vertShaderFilePath("Engine/Assets/Shaders/VertexShader.shader");
	const FilePath fragShaderFilePath("Engine/Assets/Shaders/FragmentShader.shader");

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

	defaultShader->GenerateShaderProgram();
}

void CreateTextureShader()
{
	const FilePath vertShaderFilePath("Engine/Assets/Shaders/TextureVert.shader");
	const FilePath fragShaderFilePath("Engine/Assets/Shaders/TextureFrag.shader");

	ResourceHandle vertShaderHandle = RZE_Engine::Get()->GetResourceHandler().RequestResource<Diotima::GFXShader>(vertShaderFilePath, EGLShaderType::Vertex, "TextureVertShader");
	ResourceHandle fragShaderHandle = RZE_Engine::Get()->GetResourceHandler().RequestResource<Diotima::GFXShader>(fragShaderFilePath, EGLShaderType::Fragment, "TextureFragShader");

	Diotima::GFXShader* vertShader = RZE_Engine::Get()->GetResourceHandler().GetResource<Diotima::GFXShader>(vertShaderHandle);
	vertShader->Create();
	vertShader->Compile();

	Diotima::GFXShader* fragShader = RZE_Engine::Get()->GetResourceHandler().GetResource<Diotima::GFXShader>(fragShaderHandle);
	fragShader->Create();
	fragShader->Compile();

	textureShader = new Diotima::GFXShaderGroup("TextureShader");
	textureShader->AddShader(Diotima::GFXShaderGroup::EShaderIndex::Vertex, vertShader);
	textureShader->AddShader(Diotima::GFXShaderGroup::EShaderIndex::Fragment, fragShader);

	textureShader->GenerateShaderProgram();
}

RenderSystem::RenderSystem()
{
	mMainCamera = nullptr;
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

	Diotima::Renderer::CameraItemProtocol camera;
	camera.ProjectionMat = mMainCamera->ProjectionMat;
	camera.ViewMat = mMainCamera->ViewMat;
	renderSystem->SetCamera(camera);

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

		if (matComp && matComp->Texture.IsValid())
		{
			item.Shader = textureShader;
			item.Texture2D = RZE_Engine::Get()->GetResourceHandler().GetResource<Diotima::GFXTexture2D>(matComp->Texture);
		}
		else
		{
			item.Shader = defaultShader;
		}

		// #NOTE(Josh) For the time being, until I work a proper pipeline in
		Diotima::GFXMaterial material;
		material.Color = sDefaultFragColor;
		item.Material = material;

		renderSystem->AddRenderItem(item);
	}

	Functor<void, Apollo::EntityID> LightSourceFunc([this, &handler, &renderSystem](Apollo::EntityID entity)
	{
		LightSourceComponent* const lightComp = handler.GetComponent<LightSourceComponent>(entity);
		TransformComponent* const transfComp = handler.GetComponent<TransformComponent>(entity);

		Diotima::Renderer::LightItemProtocol item;
		item.Color = lightComp->Color;
		item.Strength = lightComp->Strength;
		item.Position = transfComp->Position;

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
		AssertNotNull(matComp);

		if (!matComp->ResourcePath.GetAbsolutePath().empty())
		{
			matComp->Texture = RZE_Engine::Get()->GetResourceHandler().RequestResource<Diotima::GFXTexture2D>(matComp->ResourcePath);
		}
	});
	handler.RegisterForComponentAddNotification<MaterialComponent>(OnMaterialComponentAdded);
}

void RenderSystem::GenerateCameraMatrices()
{
	mMainCamera->ProjectionMat = Matrix4x4::CreatePerspectiveMatrix(mMainCamera->FOV, mMainCamera->AspectRatio, mMainCamera->NearCull, mMainCamera->FarCull);
	mMainCamera->ViewMat = Matrix4x4::CreateViewMatrix(mMainCamera->Position, mMainCamera->Position + mMainCamera->Forward, mMainCamera->UpDir);
}
