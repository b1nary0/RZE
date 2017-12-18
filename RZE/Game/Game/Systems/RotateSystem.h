#pragma once

#include <Apollo/ECS/EntitySystem.h>

#include <Utils/Math/Vector3D.h>

class RotateSystem : public Apollo::EntitySystem
{
public:
	RotateSystem();
	virtual ~RotateSystem();

	// EntitySystem interface
public:
	virtual void Initialize();
	virtual void Update(std::vector<Apollo::EntityID>& entities);
	virtual void ShutDown();

private:
	void BindInputs();

private:
	struct CameraComponent* mMainCamera;
	Vector3D mVelocity;
};