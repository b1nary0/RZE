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






	// #TODO(Josh) This is test function 
	void CreateEntities();

private:
	struct CameraComponent* mMainCamera;
	Vector3D mAxisSensitivity;

	Vector3D mPitchYawRoll;
	bool bIsMouseDown;

	


	// #TODO(Josh) Test entities for rotation stuff
	Apollo::EntityID mEntity0;
	Apollo::EntityID mEntity1;
};