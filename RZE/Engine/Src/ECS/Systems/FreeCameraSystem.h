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

	void RegisterComponentAddedNotifications();

private:
	float mSpeed = 0.5f;
	Vector3D mMoveToPoint;
};