#pragma once

#include <Apollo/ECS/EntitySystem.h>

struct CameraComponent;
struct TransformComponent;

class FreeCameraSystem : public Apollo::EntitySystem
{
public:
	FreeCameraSystem();
	virtual ~FreeCameraSystem();

public:
	virtual void Initialize();
	virtual void Update(std::vector<Apollo::EntityID>& entities);
	virtual void ShutDown();

private:
	void KeyboardInput(CameraComponent& camComp, TransformComponent& transfComp);
	void MouseInput(CameraComponent& camComp, TransformComponent& transfComp);

	void RegisterComponentAddedNotifications();

	Vector3D ArcBallProjection(const Vector3D& vec);

private:
	float mSpeed = 0.5f;
	Vector3D mMoveToPoint;

	// Keeping own version of previous position agnostic of the engine one because we only want to know
	// the previous position as it relates to these calculations. Maybe there is a better way but this is now.
	Vector3D mMousePrevPos;
	Vector3D mPitchYawRoll;
	Vector3D mOrbitPoint;
};