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

#include <Graphics/Material.h>
#include <Graphics/MeshGeometry.h>
#include <Graphics/Shader.h>
#include <Graphics/Texture2D.h>

#include <Diotima/Renderer.h>

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
	OPTICK_EVENT();
	// Update Renderer camera
	{
		Apollo::EntityHandler& handler = InternalGetEntityHandler();

		/* CAMERA */
		{
			TransformComponent* const transfComp = handler.GetComponent<TransformComponent>(mCurrentCameraEntity);
			CameraComponent* const camComp = handler.GetComponent<CameraComponent>(mCurrentCameraEntity);
			AssertNotNull(transfComp);
			AssertNotNull(camComp);

			GenerateCameraMatrices(*camComp, *transfComp);

			Diotima::CameraData cameraData;
			cameraData.Position = transfComp->Position;
			cameraData.ProjectionMat = camComp->ProjectionMat;
			cameraData.ViewMat = camComp->ViewMat;
			cameraData.FOV = camComp->FOV;
			cameraData.AspectRatio = camComp->AspectRatio;
			cameraData.NearCull = camComp->NearCull;
			cameraData.FarCull = camComp->FarCull;

			mRenderer->SetCameraData(cameraData);
		}

		/* XFORM UPDATE */
		{
			OPTICK_EVENT("XForm Update");
			// #TODO
			// Gross... This is obviously not the way we should be doing this..
			for (auto& rootNode : mRootNodes)
			{
				TransformComponent* const transformComponent = handler.GetComponent<TransformComponent>(rootNode.EntityID);

				// rootnodes don't have a render object
				rootNode.Transform = transformComponent->GetAsMat4x4();
				for (auto& child : rootNode.Children)
				{
					mRenderer->UpdateRenderObject(child.RenderObjectIndex, rootNode.Transform);
				}
			}
		}
	}
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

			// #TODO
			// Places like these don't need to assert. GetComponent should be (is) verifying via assert.
			// If that rule every changes just fix the callsites then. Extraneous code.
			AssertNotNull(meshComponent);
			AssertMsg(!meshComponent->Resource.IsValid(), 
				"MeshComponent resource already loaded. RenderSystem should be the only code servicing MeshComponent resource loads");
			TransformComponent* const rootTransformComponent = handler.GetComponent<TransformComponent>(entityID);
			AssertNotNull(rootTransformComponent);

			ResourceHandle resource = resourceHandler.LoadResource<Model3D>(meshComponent->ResourcePath);
			AssertExpr(resource.IsValid());
			meshComponent->Resource = resource;

			const Model3D* const modelData = resourceHandler.GetResource<Model3D>(resource);
			AssertNotNull(modelData);

			CreateAndInitializeRenderNode(entityID, *modelData, rootTransformComponent->GetAsMat4x4());
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
			CameraComponent* const camComp = handler.GetComponent<CameraComponent>(entityID);
			AssertNotNull(camComp);

			if (mCurrentCameraEntity != Apollo::kInvalidEntityID)
			{
				CameraComponent* const currentCamera = handler.GetComponent<CameraComponent>(mCurrentCameraEntity);
				AssertNotNull(currentCamera);

				currentCamera->bIsActiveCamera = false;
			}

			// #NOTE(For now, the last camera added becomes the main camera.)
			mCurrentCameraEntity = entityID;
			camComp->bIsActiveCamera = true;

			camComp->AspectRatio = RZE_Application::RZE().GetWindowSize().X() / RZE_Application::RZE().GetWindowSize().Y();
		});
		handler.RegisterForComponentAddNotification<CameraComponent>(OnCameraComponentAdded);

		Apollo::EntityHandler::ComponentModifiedFunc OnCameraComponentModified([this, &handler](Apollo::EntityID entityID)
		{
			CameraComponent* const camComp = handler.GetComponent<CameraComponent>(entityID);
			AssertNotNull(camComp);

			if (camComp->bIsActiveCamera)
			{
				if (mCurrentCameraEntity != Apollo::kInvalidEntityID)
				{
					CameraComponent* const currentCamera = handler.GetComponent<CameraComponent>(mCurrentCameraEntity);
					AssertNotNull(currentCamera);

					currentCamera->bIsActiveCamera = false;
				}

				mCurrentCameraEntity = entityID;
			}
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
	OPTICK_EVENT("GenerateCameraMatrices");

	cameraComponent.ProjectionMat = Matrix4x4::CreatePerspectiveMatrix(cameraComponent.FOV, cameraComponent.AspectRatio, cameraComponent.NearCull, cameraComponent.FarCull);
	cameraComponent.ViewMat = Matrix4x4::CreateViewMatrix(transformComponent.Position, transformComponent.Position + cameraComponent.Forward, cameraComponent.UpDir);
}

void RenderSystem::CreateAndInitializeRenderNode(const Apollo::EntityID entityID, const Model3D& modelData, const Matrix4x4& transform)
{
	ResourceHandler& resourceHandler = RZE_Application::RZE().GetResourceHandler();

	mRootNodes.emplace_back();
	RenderNode& rootNode = mRootNodes.back();
	rootNode.Transform = transform;
	rootNode.EntityID = entityID;
	for (auto& meshGeometry : modelData.GetStaticMesh().GetSubMeshes())
	{
		const Material& material = meshGeometry.GetMaterial();

		rootNode.Children.emplace_back();
		RenderNode& childNode = rootNode.Children.back();
		childNode.Geometry = &meshGeometry;

		Diotima::MeshData meshData;
		meshData.Vertices = childNode.Geometry->GetVertexDataRaw();
		meshData.Indices = childNode.Geometry->GetIndexDataRaw();

		meshData.Material.mProperties.Shininess = material.Shininess;

		const ShaderTechnique* const shader = resourceHandler.GetResource<ShaderTechnique>(material.GetShaderResource());
		AssertNotNull(shader);
		meshData.Material.mShaderID = shader->GetHardwareID();

		std::vector<Diotima::TextureData> textureData;
		textureData.reserve(Material::TEXTURE_SLOT_COUNT);
		for (size_t textureSlot = 0; textureSlot < Material::TEXTURE_SLOT_COUNT; ++textureSlot)
		{
			// #TODO We should really be writing code that can deal with permutations of valid textures.
			// Likely via a shader infrastructure that can validate the data needed and pair with its materials etc
			const Texture2D* const texture = resourceHandler.GetResource<Texture2D>(material.GetTexture(textureSlot));
			if (texture != nullptr)
			{
				Diotima::TextureData data;
				// Can solve this problem with a Vector2D(int) maybe?
				data.mHeader.mWidth = static_cast<int>(texture->GetDimensions().X());
				data.mHeader.mHeight = static_cast<int>(texture->GetDimensions().Y());
				data.mData = texture->GetRawData();
				textureData.emplace_back(std::move(data));
			}
		}

		childNode.RenderObjectIndex = mRenderer->CreateRenderObject(meshData, textureData, rootNode.Transform);
	}
}
