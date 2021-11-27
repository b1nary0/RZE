#pragma once

#include <Apollo/ECS/EntitySystem.h>

#include <Utils/Math/Vector3D.h>

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

	// Component lifecycle callbacks
	void OnTransformComponentAdded(Apollo::EntityID entityID);
private:

private:
	float mSpeed = 2.0f;
	Vector3D mMoveToPoint;
	Vector3D mMousePrevPos;
	Vector3D mYawPitchRoll;
	Apollo::EntityID mCamera;
};