#include <StdAfx.h>
#include <Game/Systems/ProjectileSystem.h>

#include <ECS/Components/CameraComponent.h>
#include <ECS/Components/LifetimeComponent.h>
#include <ECS/Components/MeshComponent.h>
#include <ECS/Components/TransformComponent.h>

#include <Game/Components/VelocityComponent.h>

void ProjectileSystem::Initialize()
{
	InternalGetComponentFilter().AddFilterType<TransformComponent>();
	InternalGetComponentFilter().AddFilterType<VelocityComponent>();

	Apollo::EntityHandler& entityHandler = InternalGetEntityHandler();

	Apollo::EntityHandler::ComponentAddedFunc onCameraAdded([this](Apollo::EntityID entity)
	{
		mCameraEntity = entity;
	});
	entityHandler.RegisterForComponentAddNotification<CameraComponent>(onCameraAdded);

	InputHandler::KeyActionFunc leftButtonFunc([this, &entityHandler](const InputKey& key)
	{
		CameraComponent* const camComp = entityHandler.GetComponent<CameraComponent>(mCameraEntity);
		TransformComponent* const camTransComp = entityHandler.GetComponent<TransformComponent>(mCameraEntity);
		AssertNotNull(camComp);

		Vector3D velocity(5.0f);
		velocity *= camComp->Forward;

		Apollo::EntityID projectileEntity = RZE_Application::RZE().GetActiveScene().CreateEntity("Projectile");
		TransformComponent* const projectileTransform = entityHandler.GetComponent<TransformComponent>(projectileEntity);
		projectileTransform->Position = camTransComp->Position;
		projectileTransform->Scale = Vector3D(0.25f, 0.25f, 0.25f);

		entityHandler.AddComponent<MeshComponent>(projectileEntity, FilePath("Assets/3D/Cube.obj"));
		entityHandler.AddComponent<VelocityComponent>(projectileEntity, velocity);
		entityHandler.AddComponent<LifetimeComponent>(projectileEntity, 3000.0f);
	});

	InputHandler& inputHandler = RZE_Application::RZE().GetInputHandler();
	inputHandler.BindAction(Win32KeyCode::Key_F, EButtonState::ButtonState_Pressed, leftButtonFunc);
}

void ProjectileSystem::Update(const std::vector<Apollo::EntityID>& entities)
{
	Apollo::EntityHandler& entityHandler = InternalGetEntityHandler();
	for (Apollo::EntityID entity : entities)
	{
		TransformComponent* const transfComp = entityHandler.GetComponent<TransformComponent>(entity);
		VelocityComponent* const velocityComp = entityHandler.GetComponent<VelocityComponent>(entity);

		transfComp->Position += velocityComp->Velocity * static_cast<float>(RZE_Application::RZE().GetDeltaTime());
	}
}

void ProjectileSystem::ShutDown()
{

}
