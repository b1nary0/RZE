//#include <Game/Systems/ProjectileSystem.h>
//
//#include <ECS/Components/CameraComponent.h>
//#include <ECS/Components/LifetimeComponent.h>
//#include <ECS/Components/MeshComponent.h>
//#include <ECS/Components/TransformComponent.h>
//
//#include <Game/Components/VelocityComponent.h>
//
//#include <Game/Model3D.h>
//
//#include <Game/GameApp.h>
//
//#include <Optick/optick.h>
//
//namespace
//{
//	constexpr char kProjectileMeshAssetPath[] = "ProjectData/Mesh/NeckMechWalker.meshasset";
//}
//
//void ProjectileSystem::Initialize()
//{
//	InternalGetComponentFilter().AddFilterType<TransformComponent>();
//	InternalGetComponentFilter().AddFilterType<VelocityComponent>();
//
//	Apollo::EntityHandler& entityHandler = InternalGetEntityHandler();
//
//	RegisterForComponentNotifications();
//
//	mProjectileResource = RZE::GetResourceHandler().LoadResource<Model3D>(FilePath(kProjectileMeshAssetPath));
//	AssertExpr(mProjectileResource.IsValid());
//	
//	InputHandler::KeyActionFunc leftButtonFunc([this, &entityHandler](const InputKey& key)
//	{
//		CameraComponent* const camComp = entityHandler.GetComponent<CameraComponent>(mCameraEntity);
//		TransformComponent* const camTransComp = entityHandler.GetComponent<TransformComponent>(mCameraEntity);
//		AssertNotNull(camComp);
//
//		Vector3D velocity(30.0f);
//		velocity *= camComp->Forward;
//
//		Apollo::EntityID projectileEntity = RZE_Application::RZE().GetActiveScene().CreateEntity("Projectile");
//		TransformComponent* const projectileTransform = entityHandler.GetComponent<TransformComponent>(projectileEntity);
//		projectileTransform->Position = camTransComp->Position - Vector3D(0.0f, 0.5f, 0.0f);
//		projectileTransform->Scale = Vector3D(0.25f, 0.25f, 0.25f);
//
//		// #TODO(Need to resolve the situation where you want to create a MeshComponent based off an existing ResourceHandle.
//		//       For the time being this will simply end up just returning the cached resource but it should be explicit)
//		entityHandler.AddComponent<MeshComponent>(projectileEntity, FilePath(kProjectileMeshAssetPath));
//		entityHandler.AddComponent<VelocityComponent>(projectileEntity, velocity);
//		entityHandler.AddComponent<LifetimeComponent>(projectileEntity, 3000.0f);
//	});
//
//	InputHandler& inputHandler = RZE_Application::RZE().GetInputHandler();
//	inputHandler.BindAction(Win32KeyCode::Key_F, EButtonState::ButtonState_Pressed, leftButtonFunc);
//}
//
//void ProjectileSystem::Update(const std::vector<Apollo::EntityID>& entities)
//{
//	OPTICK_EVENT();
//
//	Apollo::EntityHandler& entityHandler = InternalGetEntityHandler();
//	for (Apollo::EntityID entity : entities)
//	{
//		TransformComponent* const transfComp = entityHandler.GetComponent<TransformComponent>(entity);
//		VelocityComponent* const velocityComp = entityHandler.GetComponent<VelocityComponent>(entity);
//
//		transfComp->Position += velocityComp->Velocity * static_cast<float>(RZE_Application::RZE().GetDeltaTime());
//	}
//}
//
//void ProjectileSystem::ShutDown()
//{
//
//}
//
//void ProjectileSystem::RegisterForComponentNotifications()
//{
//	Apollo::EntityHandler& entityHandler = RZE_Application::RZE().GetActiveScene().GetEntityHandler();
//
//	Apollo::EntityHandler::ComponentAddedFunc onCameraComponentAdded = std::bind(&ProjectileSystem::OnCameraComponentAdded, this, std::placeholders::_1);
//	entityHandler.RegisterForComponentAddNotification<CameraComponent>(onCameraComponentAdded);
//}
//
//void ProjectileSystem::OnCameraComponentAdded(Apollo::EntityID entityID)
//{
//	mCameraEntity = entityID;
//}
