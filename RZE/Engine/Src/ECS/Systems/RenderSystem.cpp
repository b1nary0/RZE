#include <StdAfx.h>
#include <ECS/Systems/RenderSystem.h>

#include <Apollo/ECS/EntityComponentFilter.h>

#include <Diotima/Graphics/RenderTarget.h>
#include <Diotima/Graphics/GFXMaterial.h>
#include <Diotima/Graphics/GFXMesh.h>
#include <Diotima/Graphics/GFXTexture2D.h>
#include <Diotima/Shaders/ShaderPipeline.h>

#include <Graphics/Material.h>
#include <Graphics/Texture2D.h>
#include <Graphics/IndexBuffer.h>
#include <Graphics/VertexBuffer.h>

#include <ECS/Components/CameraComponent.h>
#include <ECS/Components/LightSourceComponent.h>
#include <ECS/Components/MeshComponent.h>
#include <ECS/Components/TransformComponent.h>
#include <ECS/Components/MaterialComponent.h>
#include <ECS/Components/NameComponent.h>

#include <Game/Model3D.h>

#include <Utils/Platform/FilePath.h>
#include <Utils/Platform/Timers/HiResTimer.h>

#include <Perseus/JobSystem/JobScheduler.h>

static Vector4D sDefaultFragColor(0.25f, 0.25f, 0.25f, 1.0f);

// Render helpers
//-----------------------------------------
Diotima::GFXShaderPipeline* gForwardShader;
Diotima::GFXShaderPipeline* gDepthPassShader;

// #TODO(Josh::See below)
//////////////////////////////////////////////////////////////////////////
// This is a problem. Maybe these shouldnt be resources at this level and instead we have a resource that
// is the entire shader pipeline attached to the material... Maybe can store these as a renderer-distributed thing.
ResourceHandle texVertShaderHandle;
ResourceHandle texFragShaderHandle;

ResourceHandle depthPassVertShaderHandle;
ResourceHandle depthPassFragShaderHandle;
//////////////////////////////////////////////////////////////////////////
void CreateForwardShader();
void CreateDepthPassShader();
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

	RZE_Application::RZE().GetRenderer().mForwardShader = gForwardShader;
	RZE_Application::RZE().GetRenderer().mDepthPassShader = gDepthPassShader;
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
	camera.Position = transfComp->Position;
	renderer.SetCamera(camera);
	
	//Perseus::Job::Task work([this, entities, transfComp, &renderer, &handler]()
	{
 		for (auto& entity : entities)
 		{
 			TransformComponent* const transfComp = handler.GetComponent<TransformComponent>(entity);
 
 			Diotima::Renderer::RenderItemProtocol& item = renderer.GetItemProtocolByIdx(mRenderItemEntityMap[entity]);
 			item.ModelMatrix = Matrix4x4::CreateInPlace(transfComp->Position, transfComp->Scale, transfComp->Rotation);
 		}
	}/*)*/;
	//Perseus::JobScheduler::Get().PushJob(work);

	Functor<void, Apollo::EntityID> LightSourceFunc([this, &handler, &renderer](Apollo::EntityID entity)
	{
		TransformComponent* const transfComp = handler.GetComponent<TransformComponent>(entity);
		Diotima::Renderer::LightItemProtocol& item = renderer.GetLightProtocolByIdx(mLightItemEntityMap[entity]);

		Matrix4x4 orthoProj = Matrix4x4::CreateOrthoMatrix(-10.0f, 10.0f, -10.0f, 10.0f, 1.0f, 100.0f);
		Matrix4x4 lightView = Matrix4x4::CreateViewMatrix(transfComp->Position, Vector3D(), Vector3D(0.0f, 1.0f, 0.0f));

		item.LightSpaceMatrix = orthoProj * lightView;
		item.Position = transfComp->Position;
	});
	handler.ForEach<LightSourceComponent, TransformComponent>(LightSourceFunc);
}

void RenderSystem::ShutDown()
{
}

// #TODO(Josh::The concept of a render-side material seems faulty. Maybe set it up as a set of parameters for the shader/pass)
Diotima::GFXMaterial* TEMPHACK_ConvertMaterialToGPUMaterial(const Material& material)
{
	Diotima::GFXMaterial* pMaterial = new Diotima::GFXMaterial();
	if (material.HasDiffuse())
	{
		const Texture2D& diffuse = material.GetDiffuse();
		Diotima::GFXTexture2D* gpuTexture = new Diotima::GFXTexture2D(diffuse.GetRawData(), static_cast<U32>(diffuse.GetDimensions().X()), static_cast<U32>(diffuse.GetDimensions().Y()), 0, Diotima::ETextureType::Diffuse);
		pMaterial->AddTexture(gpuTexture);
	}

	if (material.HasSpecular())
	{
		const Texture2D& specular = material.GetSpecular();
		Diotima::GFXTexture2D* gpuTexture = new Diotima::GFXTexture2D(specular.GetRawData(), static_cast<U32>(specular.GetDimensions().X()), static_cast<U32>(specular.GetDimensions().Y()), 0, Diotima::ETextureType::Specular);
		pMaterial->AddTexture(gpuTexture);
	}

	if (material.HasNormal())
	{
		const Texture2D& normal = material.GetNormal();
		Diotima::GFXTexture2D* gpuTexture = new Diotima::GFXTexture2D(normal.GetRawData(), static_cast<U32>(normal.GetDimensions().X()), static_cast<U32>(normal.GetDimensions().Y()), 0, Diotima::ETextureType::Normal);
		pMaterial->AddTexture(gpuTexture);
	}

	pMaterial->Shininess = material.Shininess;
	pMaterial->Opacity = material.Opacity;

	return pMaterial;
}

void RenderSystem::RegisterForComponentNotifications()
{
	Apollo::EntityHandler& handler = InternalGetEntityHandler();

	//
	// MeshComponent
	//
	Apollo::EntityHandler::ComponentAddedFunc OnMeshComponentAdded([this, &handler](Apollo::EntityID entityID)
	{	BROFILER_EVENT("RenderSystem::OnMeshComponentAdded");
		MeshComponent* const meshComp = handler.GetComponent<MeshComponent>(entityID);
		AssertNotNull(meshComp);
		meshComp->Resource = RZE_Application::RZE().GetResourceHandler().RequestResource<Model3D>(meshComp->ResourcePath);

		if (meshComp->Resource.IsValid())
		{
			Model3D* const modelData = RZE_Application::RZE().GetResourceHandler().GetResource<Model3D>(meshComp->Resource);

			Diotima::Renderer::RenderItemProtocol item;
			for (const MeshGeometry& mesh : modelData->GetStaticMesh().GetSubMeshes())
			{
				Diotima::Renderer::RenderItemMeshData meshData;
				meshData.VertexBuffer = mesh.GetVertexBuffer();
				meshData.IndexBuffer = mesh.GetIndexBuffer();

				AssertExpr(mesh.GetMaterial().HasDiffuse());
				meshData.TextureDescs.emplace_back(mesh.GetMaterial().GetDiffuse().GetTextureBufferID(), Diotima::Renderer::ETextureType::Diffuse);
				meshData.TextureDescs.emplace_back(mesh.GetMaterial().GetSpecular().GetTextureBufferID(), Diotima::Renderer::ETextureType::Specular);
				meshData.TextureDescs.emplace_back(mesh.GetMaterial().GetNormal().GetTextureBufferID(), Diotima::Renderer::ETextureType::Normal);

				Diotima::Renderer::RenderItemMaterialDesc matDesc;
				matDesc.Shininess = mesh.GetMaterial().Shininess;

				meshData.Material = matDesc;

				item.MeshData.push_back(meshData);
			}
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
void CreateForwardShader()
{
	const FilePath vertShaderFilePath("Assets/Shaders/TextureVert.shader");
	const FilePath fragShaderFilePath("Assets/Shaders/TextureFrag.shader");

	texVertShaderHandle = RZE_Application::RZE().GetResourceHandler().RequestResource<Diotima::GFXShader>(vertShaderFilePath, 0, "TextureVertShader");
	texFragShaderHandle = RZE_Application::RZE().GetResourceHandler().RequestResource<Diotima::GFXShader>(fragShaderFilePath, 1, "TextureFragShader");

	Diotima::GFXShader* vertShader = RZE_Application::RZE().GetResourceHandler().GetResource<Diotima::GFXShader>(texVertShaderHandle);
	vertShader->Create();
	vertShader->Compile();

	Diotima::GFXShader* fragShader = RZE_Application::RZE().GetResourceHandler().GetResource<Diotima::GFXShader>(texFragShaderHandle);
	fragShader->Create();
	fragShader->Compile();

	gForwardShader = new Diotima::GFXShaderPipeline("TextureShader");
	gForwardShader->AddShader(Diotima::GFXShaderPipeline::EShaderIndex::Vertex, vertShader);
	gForwardShader->AddShader(Diotima::GFXShaderPipeline::EShaderIndex::Fragment, fragShader);

	gForwardShader->GenerateShaderProgram();
}

void CreateDepthPassShader()
{
	const FilePath vertShaderFilePath("Assets/Shaders/DepthVert.shader");
	const FilePath fragShaderFilePath("Assets/Shaders/EmptyFrag.shader");

	depthPassVertShaderHandle = RZE_Application::RZE().GetResourceHandler().RequestResource<Diotima::GFXShader>(vertShaderFilePath, 0, "DepthPassVertShader");
	depthPassFragShaderHandle = RZE_Application::RZE().GetResourceHandler().RequestResource<Diotima::GFXShader>(fragShaderFilePath, 1, "DepthPassFragShader");

	Diotima::GFXShader* vertShader = RZE_Application::RZE().GetResourceHandler().GetResource<Diotima::GFXShader>(depthPassVertShaderHandle);
	vertShader->Create();
	vertShader->Compile();

	Diotima::GFXShader* fragShader = RZE_Application::RZE().GetResourceHandler().GetResource<Diotima::GFXShader>(depthPassFragShaderHandle);
	fragShader->Create();
	fragShader->Compile();

	gDepthPassShader = new Diotima::GFXShaderPipeline("DepthPassShaderPipeline");
	gDepthPassShader->AddShader(Diotima::GFXShaderPipeline::EShaderIndex::Vertex, vertShader);
	gDepthPassShader->AddShader(Diotima::GFXShaderPipeline::EShaderIndex::Fragment, fragShader);

	gDepthPassShader->GenerateShaderProgram();
}
