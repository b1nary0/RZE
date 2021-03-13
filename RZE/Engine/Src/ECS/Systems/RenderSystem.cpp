#include <StdAfx.h>
#include <ECS/Systems/RenderSystem.h>

#include <Apollo/ECS/EntityComponentFilter.h>

#include <ECS/Components/CameraComponent.h>
#include <ECS/Components/LightSourceComponent.h>
#include <ECS/Components/MeshComponent.h>
#include <ECS/Components/TransformComponent.h>
#include <ECS/Components/MaterialComponent.h>
#include <ECS/Components/NameComponent.h>

#include <Game/Model3D.h>

#include <Utils/Platform/FilePath.h>
#include <Utils/Platform/Timers/HiResTimer.h>

#include <Utils/MemoryUtils.h>

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

	mRenderer = &RZE_Application::RZE().GetRenderer();
}

void RenderSystem::Update(const std::vector<Apollo::EntityID>& entities)
{
	
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
	{
		Apollo::EntityHandler::ComponentAddedFunc OnMeshComponentAdded([this, &handler](Apollo::EntityID entityID)
		{
			OPTICK_EVENT("RenderSystem::OnMeshComponentAdded");

			ResourceHandler& resourceHandler = RZE_Application::RZE().GetResourceHandler();

			// We've detected a MeshComponent was created. Create a render object
			// and fill it with the data required to properly render the mesh resource.
			MeshComponent* const meshComponent = handler.GetComponent<MeshComponent>(entityID);
			AssertNotNull(meshComponent);

			ResourceHandle resource = resourceHandler.LoadResource<Model3D>(meshComponent->ResourcePath);
			AssertExpr(resource.IsValid());
			meshComponent->Resource = resource;

			Model3D* const modelData = resourceHandler.GetResource<Model3D>(resource);
			AssertNotNull(modelData);


			Diotima::RenderObjectHandle objectHandle = mRenderer->CreateRenderObject();
		});
		handler.RegisterForComponentAddNotification<MeshComponent>(OnMeshComponentAdded);

		Apollo::EntityHandler::ComponentRemovedFunc OnMeshComponentRemoved([this, &handler](Apollo::EntityID entityID)
		{
		});
		handler.RegisterForComponentRemovedNotification<MeshComponent>(OnMeshComponentRemoved);

		//#TODO(Should make a function that does the common work here since this is exactly the same for added mesh except we modify existing RenderItem instead of creating one)
		Apollo::EntityHandler::ComponentModifiedFunc OnMeshComponentModified([this, &handler](Apollo::EntityID entityID)
		{
			// #TODO
			// There is a leak in this function. We don't clean up the old stuff (material buffers, etc). Fix this.
			// Best fix is probably moving away from internal usage of U32 buffer indexes to store buffer references.
			// Just use straight pointers atm.
			OPTICK_EVENT("RenderSystem::OnMeshComponentModified");
		});
		handler.RegisterForComponentModifiedNotification<MeshComponent>(OnMeshComponentModified);
	}

	// LightSourceComponent
	{
		Apollo::EntityHandler::ComponentAddedFunc OnLightSourceComponentAdded([this, &handler](Apollo::EntityID entityID)
		{
		});
		handler.RegisterForComponentAddNotification<LightSourceComponent>(OnLightSourceComponentAdded);

		Apollo::EntityHandler::ComponentRemovedFunc OnLightSourceComponentRemoved([this, &handler](Apollo::EntityID entityID)
		{
		});
		handler.RegisterForComponentRemovedNotification<LightSourceComponent>(OnLightSourceComponentRemoved);
	}

	//
	// CameraComponent
	//
	{
		Apollo::EntityHandler::ComponentAddedFunc OnCameraComponentAdded([this, &handler](Apollo::EntityID entityID)
		{
		});
		handler.RegisterForComponentAddNotification<CameraComponent>(OnCameraComponentAdded);

		Apollo::EntityHandler::ComponentModifiedFunc OnCameraComponentModified([this, &handler](Apollo::EntityID entityID)
		{
		});
		handler.RegisterForComponentModifiedNotification<CameraComponent>(OnCameraComponentModified);

		Apollo::EntityHandler::ComponentModifiedFunc OnCameraComponentRemoved([this, &handler](Apollo::EntityID entityID)
		{
		});
		handler.RegisterForComponentRemovedNotification<CameraComponent>(OnCameraComponentRemoved);
	}
}

void RenderSystem::GenerateCameraMatrices(CameraComponent& cameraComponent, const TransformComponent& transformComponent)
{
	cameraComponent.ProjectionMat = Matrix4x4::CreatePerspectiveMatrix(cameraComponent.FOV, cameraComponent.AspectRatio, cameraComponent.NearCull, cameraComponent.FarCull);
	cameraComponent.ViewMat = Matrix4x4::CreateViewMatrix(transformComponent.Position, transformComponent.Position + cameraComponent.Forward, cameraComponent.UpDir);
}
