#include <StdAfx.h>
#include <ECS/Systems/RenderSystem.h>

#include <Apollo/ECS/EntityComponentFilter.h>

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

RenderSystem::RenderSystem(Apollo::EntityHandler* const entityHandler)
	: Apollo::EntitySystem(entityHandler)
{

}

void RenderSystem::Initialize()
{
	InternalGetComponentFilter().AddFilterType<TransformComponent>();
	InternalGetComponentFilter().AddFilterType<MeshComponent>();

	RegisterForComponentNotifications();
}

void RenderSystem::Update(const std::vector<Apollo::EntityID>& entities)
{
	OPTICK_EVENT();

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
	
	size_t jobSize = entities.size() / 2;
	std::vector<Apollo::EntityID> workItems0;
	workItems0.reserve(jobSize);
	std::copy(entities.begin(), entities.begin() + jobSize, std::back_inserter(workItems0));
	Perseus::Job::Task work0([this, workItems0, &renderer, &handler]()
	{
		OPTICK_EVENT("RenderSystem Matrix Update 0");
 		for (auto& entity : workItems0)
 		{
 			TransformComponent* const transfComp = handler.GetComponent<TransformComponent>(entity);
 
 			Diotima::Renderer::RenderItemProtocol& item = renderer.GetItemProtocolByIdx(mRenderItemEntityMap[entity]);
 			item.ModelMatrix = Matrix4x4::CreateInPlace(transfComp->Position, transfComp->Scale, transfComp->Rotation);
 		}
	});

	std::vector<Apollo::EntityID> workItems1;
	workItems1.reserve(jobSize);
	std::copy(entities.begin() + jobSize, entities.end(), std::back_inserter(workItems1));
	Perseus::Job::Task work1([this, workItems1, &renderer, &handler]()
	{
		OPTICK_EVENT("RenderSystem Matrix Update 1");
		for (auto& entity : workItems1)
		{
			TransformComponent* const transfComp = handler.GetComponent<TransformComponent>(entity);

			Diotima::Renderer::RenderItemProtocol& item = renderer.GetItemProtocolByIdx(mRenderItemEntityMap[entity]);
			item.ModelMatrix = Matrix4x4::CreateInPlace(transfComp->Position, transfComp->Scale, transfComp->Rotation);
		}
	});

	Perseus::JobScheduler::Get().PushJob(work0);
	Perseus::JobScheduler::Get().PushJob(work1);
	Perseus::JobScheduler::Get().Wait();

	Functor<void, Apollo::EntityID> LightSourceFunc([this, &handler, &renderer](Apollo::EntityID entity)
	{
		LightSourceComponent* const lightComp = handler.GetComponent<LightSourceComponent>(entity);
		TransformComponent* const transfComp = handler.GetComponent<TransformComponent>(entity);

		Diotima::Renderer::LightItemProtocol& item = renderer.GetLightProtocolByIdx(mLightItemEntityMap[entity]);

		Matrix4x4 orthoProj = Matrix4x4::CreateOrthoMatrix(-100.0f, 100.0f, -100.0f, 100.0f, -100.0f, 100.0f);
		Matrix4x4 lightView = Matrix4x4::CreateViewMatrix(transfComp->Position, Vector3D(), Vector3D(0.0f, 1.0f, 0.0f));

		item.LightSpaceMatrix = orthoProj * lightView;
		item.Color = lightComp->Color;
		item.Strength = lightComp->Strength;
		item.LightType = static_cast<Diotima::Renderer::ELightType>(lightComp->LightType);
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
		OPTICK_EVENT("RenderSystem::OnMeshComponentAdded");

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

	//#TODO(Should make a function that does the common work here since this is exactly the same for added mesh except we modify existing RenderItem instead of creating one)
	Apollo::EntityHandler::ComponentModifiedFunc OnMeshComponentModified([this, &handler](Apollo::EntityID entityID)
	{
		OPTICK_EVENT("RenderSystem::OnMeshComponentModified");

		MeshComponent* const meshComp = handler.GetComponent<MeshComponent>(entityID);
		AssertNotNull(meshComp);
		// #TODO(Doing this here because ResourceHandle lifetime is buggy (copies/moves/etc))
		RZE_Application::RZE().GetResourceHandler().ReleaseResource(meshComp->Resource);
		meshComp->Resource = RZE_Application::RZE().GetResourceHandler().RequestResource<Model3D>(meshComp->ResourcePath);

		if (meshComp->Resource.IsValid())
		{
			Int32 renderIndex = mRenderItemEntityMap[entityID];
			Diotima::Renderer::RenderItemProtocol& renderItem = RZE_Application::RZE().GetRenderer().GetItemProtocolByIdx(renderIndex);
			renderItem.MeshData.clear();

			Model3D* const modelData = RZE_Application::RZE().GetResourceHandler().GetResource<Model3D>(meshComp->Resource);

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

				renderItem.MeshData.push_back(meshData);
			}
		}
	});
	handler.RegisterForComponentModifiedNotification<MeshComponent>(OnMeshComponentModified);

	// LightSourceComponent
	Apollo::EntityHandler::ComponentAddedFunc OnLightSourceComponentAdded([this, &handler](Apollo::EntityID entityID)
	{
		LightSourceComponent* const lightComp = handler.GetComponent<LightSourceComponent>(entityID);
		AssertNotNull(lightComp);

		Diotima::Renderer::LightItemProtocol item;
		item.LightType = static_cast<Diotima::Renderer::ELightType>(lightComp->LightType);
		item.Color = lightComp->Color;
		item.Strength = lightComp->Strength;

		Int32 itemIdx = RZE_Application::RZE().GetRenderer().AddLightItem(item);
		mLightItemEntityMap[entityID] = itemIdx;
	});
	handler.RegisterForComponentAddNotification<LightSourceComponent>(OnLightSourceComponentAdded);

	Apollo::EntityHandler::ComponentRemovedFunc OnLightSourceComponentRemoved([this, &handler](Apollo::EntityID entityID)
	{
		Int32 lightIndex = mLightItemEntityMap[entityID];
		RZE_Application::RZE().GetRenderer().RemoveLightItem(lightIndex);
		mLightItemEntityMap[entityID] = -1;
	});
	handler.RegisterForComponentRemovedNotification<LightSourceComponent>(OnLightSourceComponentRemoved);

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
}

void RenderSystem::GenerateCameraMatrices(CameraComponent& cameraComponent, const TransformComponent& transformComponent)
{
	cameraComponent.ProjectionMat = Matrix4x4::CreatePerspectiveMatrix(cameraComponent.FOV, cameraComponent.AspectRatio, cameraComponent.NearCull, cameraComponent.FarCull);
	cameraComponent.ViewMat = Matrix4x4::CreateViewMatrix(transformComponent.Position, transformComponent.Position + cameraComponent.Forward, cameraComponent.UpDir);
}
