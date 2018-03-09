#pragma once

#include <Apollo/ECS/EntitySystem.h>

#include <Utils/Math/Vector3D.h>

class WeirdTempInputSystem : public Apollo::EntitySystem
{
public:
	WeirdTempInputSystem();
	virtual ~WeirdTempInputSystem();

	// EntitySystem interface
public:
	virtual void Initialize();
	virtual void Update(std::vector<Apollo::EntityID>& entities);
	virtual void ShutDown();

private:
	void BindInputs();

private:
	struct CameraComponent* mMainCamera;
	Vector3D mAxisSensitivity;

	Vector3D mPitchYawRoll;
	bool bIsMouseDown;
};