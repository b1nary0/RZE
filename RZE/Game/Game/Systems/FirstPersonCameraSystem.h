#pragma once

#include <Apollo/ECS/EntitySystem.h>

struct CameraComponent;
struct TransformComponent;

class FirstPersonCameraSystem : public Apollo::EntitySystem
{
public:
	FirstPersonCameraSystem(Apollo::EntityHandler* const entityHandler);
	virtual ~FirstPersonCameraSystem();

	// EntitySystem interface
public:
	virtual void Initialize() override;
	virtual void Update(const std::vector<Apollo::EntityID>& entities) override;
	virtual void ShutDown() override;

private:
	void RegisterForComponentNotifications();

	void DoInput(CameraComponent& camera, TransformComponent& transform);

private:
	float mSpeed = 3.0f;
	Vector3D mMoveToPoint;
};