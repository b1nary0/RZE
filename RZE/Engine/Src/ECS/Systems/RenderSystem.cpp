#include <StdAfx.h>
#include <ECS/Systems/RenderSystem.h>

#include <Apollo/ECS/EntityComponentFilter.h>

#include <Diotima/RenderBatch.h>
#include <Diotima/Graphics/Material.h>
#include <Diotima/Graphics/Texture2D.h>
#include <Diotima/Shaders/ShaderPipeline.h>

#include <ECS/Components/CameraComponent.h>
#include <ECS/Components/LightSourceComponent.h>
#include <ECS/Components/MeshComponent.h>
#include <ECS/Components/TransformComponent.h>
#include <ECS/Components/MaterialComponent.h>
#include <ECS/Components/NameComponent.h>

#include <Game/Model.h>

#include <Utils/Platform/FilePath.h>
#include <Utils/Platform/Timers/HiResTimer.h>

#include <Perseus/JobSystem/JobScheduler.h>

static Vector4D sDefaultFragColor(0.25f, 0.25f, 0.25f, 1.0f);

// Render helpers
//-----------------------------------------
Diotima::GFXShaderPipeline* textureShader;
//////////////////////////////////////////////////////////////////////////
// This is a problem. Maybe these shouldnt be resources at this level and instead we have a resource that
// is the entire shader pipeline attached to the material... Maybe can store these as a renderer-distributed thing.
ResourceHandle texVertShaderHandle;
ResourceHandle texFragShaderHandle;
//////////////////////////////////////////////////////////////////////////
void CreateTextureShader();
//-----------------------------------------

RenderSystem::RenderSystem(Apollo::EntityHandler* const entityHandler)
	: Apollo::EntitySystem(entityHandler)
{

}

void RenderSystem::Initialize()
{
	InternalGetComponentFilter().AddFilterType<TransformComponent>();
	InternalGetComponentFilter().AddFilterType<MeshComponent>();

	RegisterForComponentNotifications();

	CreateTextureShader();

	RZE_Application::RZE().GetRenderer().mShaderPipeline = textureShader;
}

void RenderSystem::Update(const std::vector<Apollo::EntityID>& entities)
{	BROFILER_CATEGORY("RenderSystem::Update", Profiler::Color::Yellow)
	Apollo::EntityHandler& handler = InternalGetEntityHandler();
	Diotima::Renderer& renderer = RZE_Application::RZE().GetRenderer();

	TransformComponent* const transfComp = handler.GetComponent<TransformComponent>(mMainCameraEntity);
	CameraComponent* const camComp = handler.GetComponent<CameraComponent>(mMainCameraEntity);
	AssertNotNull(transfComp);
	AssertNotNull(camComp);

	GenerateCameraMatrices(*camComp, *transfComp);

	Diotima::Renderer::CameraItemProtocol camera;
	camera.ProjectionMat = camComp->ProjectionMat;
	camera.ViewMat = camComp->ViewMat;
	renderer.SetCamera(camera);

	Perseus::Job::Task work([this, entities, transfComp, &renderer, &handler]()
	{
		for (auto& entity : entities)
		{
			TransformComponent* const transfComp = handler.GetComponent<TransformComponent>(entity);

			Diotima::Renderer::RenderItemProtocol& item = renderer.GetItemProtocolByIdx(mRenderItemEntityMap[entity]);
			item.ModelMat = Matrix4x4::CreateInPlace(transfComp->Position, transfComp->Scale, transfComp->Rotation);
		}
	});
	Perseus::JobScheduler::Get().PushJob(work);

	Functor<void, Apollo::EntityID> LightSourceFunc([this, &handler, &renderer](Apollo::EntityID entity)
	{
		TransformComponent* const transfComp = handler.GetComponent<TransformComponent>(entity);

		Diotima::Renderer::LightItemProtocol& item = renderer.GetLightProtocolByIdx(mLightItemEntityMap[entity]);
		item.Position = transfComp->Position;
	});

	handler.ForEach<LightSourceComponent, TransformComponent>(LightSourceFunc);
}

void RenderSystem::ShutDown()
{
}

void RenderSystem::RegisterForComponentNotifications()
{
	Apollo::EntityHandler& handler = InternalGetEntityHandler();

	//
	// MeshComponent
	//
	Apollo::EntityHandler::ComponentAddedFunc OnMeshComponentAdded([this, &handler](Apollo::EntityID entityID)
	{
		MeshComponent* const meshComp = handler.GetComponent<MeshComponent>(entityID);
		AssertNotNull(meshComp);
		meshComp->Resource = RZE_Application::RZE().GetResourceHandler().RequestResource<Model3D>(meshComp->ResourcePath);

		if (meshComp->Resource.IsValid())
		{
			Diotima::Renderer::RenderItemProtocol item;

			Model3D* const modelData = RZE_Application::RZE().GetResourceHandler().GetResource<Model3D>(meshComp->Resource);

			size_t numTextures = modelData->GetTextureHandles().size();
			if (numTextures > 0)
			{
				std::vector<Diotima::GFXTexture2D*> textures;
				textures.reserve(numTextures);
				for (size_t i = 0; i < numTextures; ++i)
				{
					textures.push_back(RZE_Application::RZE().GetResourceHandler().GetResource<Diotima::GFXTexture2D>(modelData->GetTextureHandles()[i]));
				}
				item.Textures = std::move(textures);
			}
			item.BatchData = modelData->GetRenderBatch();

			Int32 itemIdx = RZE_Application::RZE().GetRenderer().AddRenderItem(item);
			mRenderItemEntityMap[entityID] = itemIdx;
		}
	});
	handler.RegisterForComponentAddNotification<MeshComponent>(OnMeshComponentAdded);

	// LightSourceComponent
	Apollo::EntityHandler::ComponentAddedFunc OnLightSourceComponentAdded([this, &handler](Apollo::EntityID entityID)
	{
		LightSourceComponent* const lightComp = handler.GetComponent<LightSourceComponent>(entityID);
		AssertNotNull(lightComp);

		Diotima::Renderer::LightItemProtocol item;
		item.Color = lightComp->Color;
		item.Strength = lightComp->Strength;

		Int32 itemIdx = RZE_Application::RZE().GetRenderer().AddLightItem(item);
		mLightItemEntityMap[entityID] = itemIdx;
	});
	handler.RegisterForComponentAddNotification<LightSourceComponent>(OnLightSourceComponentAdded);

	//
	// CameraComponent
	//
	Apollo::EntityHandler::ComponentAddedFunc OnCameraComponentAdded([this, &handler](Apollo::EntityID entityID)
	{
		CameraComponent* const camComp = handler.GetComponent<CameraComponent>(entityID);
		AssertNotNull(camComp);

		// #TODO(Josh) Will/should be removed when a better tracking system for main cameras exist. For now since we're only working with one camera
		// for the forseeable future, its fine.
		mMainCameraEntity = entityID;
		camComp->bIsActiveCamera = true;

		camComp->AspectRatio = RZE_Application::RZE().GetWindowSize().X() / RZE_Application::RZE().GetWindowSize().Y();
	});
	handler.RegisterForComponentAddNotification<CameraComponent>(OnCameraComponentAdded);

	Apollo::EntityHandler::ComponentRemovedFunc OnMeshComponentRemoved([this, &handler](Apollo::EntityID entityID)
	{
		// #TODO(Josh) Is this the best way? Should the component hold logic to clean itself up or should it be entirely just data and the systems worry about cleanup?
		//					[Answer]  Current thoughts leave it this way, absolutely logic should be left to the systems. Impose a proper lifecycle protocol.
		MeshComponent* const meshComponent = handler.GetComponent<MeshComponent>(entityID);
		AssertNotNull(meshComponent);

		RZE_Application::RZE().GetResourceHandler().ReleaseResource(meshComponent->Resource);

		Int32 renderIndex = mRenderItemEntityMap[entityID];
		RZE_Application::RZE().GetRenderer().RemoveRenderItem(renderIndex);
		mRenderItemEntityMap[entityID] = -1;
	});
	handler.RegisterForComponentRemovedNotification<MeshComponent>(OnMeshComponentRemoved);
}

void RenderSystem::GenerateCameraMatrices(CameraComponent& cameraComponent, const TransformComponent& transformComponent)
{
	cameraComponent.ProjectionMat = Matrix4x4::CreatePerspectiveMatrix(cameraComponent.FOV, cameraComponent.AspectRatio, cameraComponent.NearCull, cameraComponent.FarCull);
	cameraComponent.ViewMat = Matrix4x4::CreateViewMatrix(transformComponent.Position, transformComponent.Position + cameraComponent.Forward, cameraComponent.UpDir);
}



// -------------------------------------------------------------------------------



/////////////
/////////// These are just dev helpers until the time of the great render comes along
//////
void CreateTextureShader()
{
	const FilePath vertShaderFilePath("Engine/Assets/Shaders/TextureVert.shader");
	const FilePath fragShaderFilePath("Engine/Assets/Shaders/TextureFrag.shader");

	texVertShaderHandle = RZE_Application::RZE().GetResourceHandler().RequestResource<Diotima::GFXShader>(vertShaderFilePath, EGLShaderType::Vertex, "TextureVertShader");
	texFragShaderHandle = RZE_Application::RZE().GetResourceHandler().RequestResource<Diotima::GFXShader>(fragShaderFilePath, EGLShaderType::Fragment, "TextureFragShader");

	Diotima::GFXShader* vertShader = RZE_Application::RZE().GetResourceHandler().GetResource<Diotima::GFXShader>(texVertShaderHandle);
	vertShader->Create();
	vertShader->Compile();

	Diotima::GFXShader* fragShader = RZE_Application::RZE().GetResourceHandler().GetResource<Diotima::GFXShader>(texFragShaderHandle);
	fragShader->Create();
	fragShader->Compile();

	textureShader = new Diotima::GFXShaderPipeline("TextureShader");
	textureShader->AddShader(Diotima::GFXShaderPipeline::EShaderIndex::Vertex, vertShader);
	textureShader->AddShader(Diotima::GFXShaderPipeline::EShaderIndex::Fragment, fragShader);

	textureShader->GenerateShaderProgram();
}
