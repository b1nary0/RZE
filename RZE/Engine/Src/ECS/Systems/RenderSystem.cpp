#include <StdAfx.h>

#include <ECS/Systems/RenderSystem.h>

#include <ECS/Components/CameraComponent.h>
#include <ECS/Components/LightSourceComponent.h>
#include <ECS/Components/MeshComponent.h>
#include <ECS/Components/TransformComponent.h>
#include <ECS/Components/MaterialComponent.h>

#include <Apollo/ECS/EntityComponentFilter.h>

#include <Game/Model.h>

#include <Diotima/Graphics/Material.h>
#include <Diotima/Graphics/Texture2D.h>
#include <Diotima/Shaders/ShaderGroup.h>

#include <Utils/Platform/FilePath.h>
#include <Utils/Platform/Timers/HiResTimer.h>

static Vector4D sDefaultFragColor(0.25f, 0.25f, 0.25f, 1.0f);

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
		TransformComponent* const transfComp = handler.GetComponent<TransformComponent>(entity);

		Diotima::Renderer::RenderItemProtocol& item = renderSystem->GetItemProtocolByIdx(mRenderItemEntityMap[entity]);

		Matrix4x4 modelMat;
		modelMat.Translate(transfComp->Position);
		modelMat.Rotate(transfComp->Rotation.ToAngle(), transfComp->Rotation.ToAxis());
		modelMat.Scale(transfComp->Scale);

		item.ModelMat = modelMat;
	}


	Functor<void, Apollo::EntityID> LightSourceFunc([this, &handler, &renderSystem](Apollo::EntityID entity)
	{
		TransformComponent* const transfComp = handler.GetComponent<TransformComponent>(entity);

		Diotima::Renderer::LightItemProtocol& item = renderSystem->GetLightProtocolByIdx(mLightItemEntityMap[entity]);
		item.Position = transfComp->Position;
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
		meshComp->Resource = RZE_Engine::Get()->GetResourceHandler().RequestResource<Model3D>(meshComp->ResourcePath);

		Diotima::Renderer::RenderItemProtocol item;

		if (meshComp->Resource.IsValid())
		{
			Model3D* const modelData = RZE_Engine::Get()->GetResourceHandler().GetResource<Model3D>(meshComp->Resource);

			item.MeshData = &modelData->GetMeshList();
			item.Shader = textureShader;

			size_t numTextures = modelData->GetTextureHandles().size();
			if (numTextures > 0)
			{
				std::vector<Diotima::GFXTexture2D*> textures(numTextures);
				for (size_t i = 0; i < numTextures; ++i)
				{
					textures.push_back(RZE_Engine::Get()->GetResourceHandler().GetResource<Diotima::GFXTexture2D>(modelData->GetTextureHandles()[i]));
				}
				item.Textures = std::move(textures);
			}
			else
			{
				item.Shader = defaultShader;
			}
		}

		item.Material.Color = sDefaultFragColor;

		Int32 itemIdx = RZE_Engine::Get()->GetRenderSystem()->AddRenderItem(item);
		mRenderItemEntityMap[entityID] = itemIdx;
	});
	handler.RegisterForComponentAddNotification<MeshComponent>(OnMeshComponentAdded);

	// LightSourceComponent
	Apollo::ComponentHandler::ComponentAddedFunc OnLightSourceComponentAdded([this](Apollo::EntityID entityID, Apollo::ComponentHandler& handler)
	{
		LightSourceComponent* const lightComp = handler.GetComponent<LightSourceComponent>(entityID);

		Diotima::Renderer::LightItemProtocol item;
		item.Color = lightComp->Color;
		item.Strength = lightComp->Strength;

		Int32 itemIdx = RZE_Engine::Get()->GetRenderSystem()->AddLightItem(item);
		mLightItemEntityMap[entityID] = itemIdx;
	});
	handler.RegisterForComponentAddNotification<LightSourceComponent>(OnLightSourceComponentAdded);

	//
	// CameraComponent
	//
	Apollo::ComponentHandler::ComponentAddedFunc OnCameraComponentAdded([this](Apollo::EntityID entityID, Apollo::ComponentHandler& handler)
	{
		this->mMainCamera = handler.GetComponent<CameraComponent>(entityID);
	});
	handler.RegisterForComponentAddNotification<CameraComponent>(OnCameraComponentAdded);
}

void RenderSystem::GenerateCameraMatrices()
{
	mMainCamera->ProjectionMat = Matrix4x4::CreatePerspectiveMatrix(mMainCamera->FOV, mMainCamera->AspectRatio, mMainCamera->NearCull, mMainCamera->FarCull);
	mMainCamera->ViewMat = Matrix4x4::CreateViewMatrix(mMainCamera->Position, mMainCamera->Position + mMainCamera->Forward, mMainCamera->UpDir);
}
