#include <StdAfx.h>

#include <ECS/Systems/RenderSystem.h>
#include <Apollo/ECS/EntityComponentFilter.h>

#include <ECS/Components/CameraComponent.h>
#include <ECS/Components/LightSourceComponent.h>
#include <ECS/Components/MeshComponent.h>
#include <ECS/Components/TransformComponent.h>
#include <Game/Model3D.h>

#include <Graphics/Material.h>
#include <Graphics/MeshGeometry.h>
#include <Graphics/Shader.h>
#include <Graphics/Texture2D.h>

#include <Diotima/Renderer.h>

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
		Apollo::EntityHandler::ComponentAddedFunc onMeshComponentAdded = std::bind(&RenderSystem::OnMeshComponentAdded, this, std::placeholders::_1);
		handler.RegisterForComponentAddNotification<MeshComponent>(onMeshComponentAdded);
		
		Apollo::EntityHandler::ComponentRemovedFunc onMeshComponentRemoved = std::bind(&RenderSystem::OnMeshComponentRemoved, this, std::placeholders::_1);
		handler.RegisterForComponentRemovedNotification<MeshComponent>(onMeshComponentRemoved);

		Apollo::EntityHandler::ComponentModifiedFunc onMeshComponentModified = std::bind(&RenderSystem::OnMeshComponentModified, this, std::placeholders::_1);
		handler.RegisterForComponentModifiedNotification<MeshComponent>(onMeshComponentModified);
	}

	//
	// CameraComponent
	//
	{
		Apollo::EntityHandler::ComponentAddedFunc onCameraComponentAdded = std::bind(&RenderSystem::OnCameraComponentAdded, this, std::placeholders::_1);
		handler.RegisterForComponentAddNotification<CameraComponent>(onCameraComponentAdded);

		Apollo::EntityHandler::ComponentRemovedFunc onCameraComponentRemoved = std::bind(&RenderSystem::OnCameraComponentRemoved, this, std::placeholders::_1);
		handler.RegisterForComponentRemovedNotification<CameraComponent>(onCameraComponentRemoved);

		Apollo::EntityHandler::ComponentModifiedFunc onCameraComponentModified = std::bind(&RenderSystem::OnCameraComponentModified, this, std::placeholders::_1);
		handler.RegisterForComponentModifiedNotification<CameraComponent>(onCameraComponentModified);
	}
}

void RenderSystem::OnMeshComponentAdded(Apollo::EntityID entityID)
{
	OPTICK_EVENT("RenderSystem::OnMeshComponentAdded");

	Apollo::EntityHandler& handler = RZE_Application::RZE().GetActiveScene().GetEntityHandler();
	ResourceHandler& resourceHandler = RZE::GetResourceHandler();

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
}

void RenderSystem::OnMeshComponentRemoved(Apollo::EntityID entityID)
{
}

void RenderSystem::OnMeshComponentModified(Apollo::EntityID entityID)
{
}

void RenderSystem::OnCameraComponentAdded(Apollo::EntityID entityID)
{
	Apollo::EntityHandler& entityHandler = RZE_Application::RZE().GetActiveScene().GetEntityHandler();

	CameraComponent* const camComp = entityHandler.GetComponent<CameraComponent>(entityID);
	AssertNotNull(camComp);

	if (mCurrentCameraEntity != Apollo::kInvalidEntityID)
	{
		CameraComponent* const currentCamera = entityHandler.GetComponent<CameraComponent>(mCurrentCameraEntity);
		AssertNotNull(currentCamera);

		currentCamera->bIsActiveCamera = false;
	}

	// #NOTE(For now, the last camera added becomes the main camera.)
	mCurrentCameraEntity = entityID;
	camComp->bIsActiveCamera = true;

	camComp->AspectRatio = RZE_Application::RZE().GetWindowSize().X() / RZE_Application::RZE().GetWindowSize().Y();
}

void RenderSystem::OnCameraComponentRemoved(Apollo::EntityID entityID)
{
}

void RenderSystem::OnCameraComponentModified(Apollo::EntityID entityID)
{
	Apollo::EntityHandler& entityHandler = RZE_Application::RZE().GetActiveScene().GetEntityHandler();

	CameraComponent* const camComp = entityHandler.GetComponent<CameraComponent>(entityID);
	AssertNotNull(camComp);

	if (camComp->bIsActiveCamera)
	{
		if (mCurrentCameraEntity != Apollo::kInvalidEntityID)
		{
			CameraComponent* const currentCamera = entityHandler.GetComponent<CameraComponent>(mCurrentCameraEntity);
			AssertNotNull(currentCamera);

			currentCamera->bIsActiveCamera = false;
		}

		mCurrentCameraEntity = entityID;
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
	ResourceHandler& resourceHandler = RZE::GetResourceHandler();

	mRootNodes.emplace_back();
	RenderNode& rootNode = mRootNodes.back();
	rootNode.NodeIndex = mRootNodes.size() - 1;

	rootNode.Transform = transform;
	rootNode.EntityID = entityID;
	for (auto& meshGeometry : modelData.GetStaticMesh().GetSubMeshes())
	{
		const Material& material = meshGeometry.GetMaterial();

		rootNode.Children.emplace_back();
		RenderNode& childNode = rootNode.Children.back();

		Diotima::MeshData meshData;
		meshData.Vertices = meshGeometry.GetVertexDataRaw();
		meshData.Indices = meshGeometry.GetIndexDataRaw();

		meshData.Material.mProperties.Shininess = material.Shininess;

		ResourceHandle testResource = resourceHandler.LoadResource<ShaderTechnique>(FilePath("Assets/Shaders/Pixel_Default_NewRenderer.hlsl"), "Default");
		const ShaderTechnique* const shader = resourceHandler.GetResource<ShaderTechnique>(testResource);//resourceHandler.GetResource<ShaderTechnique>(material.GetShaderResource());
		AssertNotNull(shader);
		meshData.Material.mShaderID = shader->GetHardwareID();

		std::vector<Diotima::TextureData> textureData;
		textureData.reserve(Material::TEXTURE_SLOT_COUNT);
// 		for (size_t textureSlot = 0; textureSlot < Material::TEXTURE_SLOT_COUNT; ++textureSlot)
// 		{
// 			// #TODO We should really be writing code that can deal with permutations of valid textures.
// 			// Likely via a shader infrastructure that can validate the data needed and pair with its materials etc
// 			const Texture2D* const texture = resourceHandler.GetResource<Texture2D>(material.GetTexture(textureSlot));
// 			if (texture != nullptr)
// 			{
// 				Diotima::TextureData data;
// 				// Can solve this problem with a Vector2D(int) maybe?
// 				data.mHeader.mWidth = static_cast<int>(texture->GetDimensions().X());
// 				data.mHeader.mHeight = static_cast<int>(texture->GetDimensions().Y());
// 				data.mData = texture->GetRawData();
// 				textureData.emplace_back(std::move(data));
// 			}
// 		}

		childNode.RenderObjectIndex = mRenderer->CreateAndInitializeRenderObject(meshData, textureData, rootNode.Transform);
	}
}
