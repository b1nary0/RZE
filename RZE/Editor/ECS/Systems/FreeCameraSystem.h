#pragma once

#include <Apollo/ECS/EntitySystem.h>

#include <Utils/Math/Vector3D.h>

struct CameraComponent;
struct TransformComponent;

class FreeCameraSystem final : public Apollo::EntitySystem
{
public:
	FreeCameraSystem(Apollo::EntityHandler* const entityHandler);
	virtual ~FreeCameraSystem();

public:
	virtual void Initialize() override;
	virtual void Update(const std::vector<Apollo::EntityID>& entities) override;
	virtual void ShutDown() override;

private:
	void KeyboardInput(CameraComponent& camComp, TransformComponent& transfComp);
	void MouseInput(CameraComponent& camComp, TransformComponent& transfComp);

	void RegisterComponentAddedNotifications();

	Vector3D ArcBallProjection(const Vector3D& vec);

private:
	float mSpeed = 2.5f;
	float mWheelZoomSpeed = 1.5f;
	Vector3D mMoveToPoint;

	// Keeping own version of previous position agnostic of the engine one because we only want to know
	// the previous position as it relates to these calculations. Maybe there is a better way but this is now.
	Vector3D mMousePrevPos;
	Vector3D mPitchYawRoll;
	Vector3D mOrbitPoint;
};