#pragma once

#include <Apollo/ECS/EntitySystem.h>


#include <Utils/Math/Vector2D.h>
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
	Vector3D ArcBallProjection(const Vector3D& vec);

private:
	Apollo::EntityID mMainCamera;
	
	Vector3D mPitchYawRoll;
	Vector3D mOrbitPoint;


	// #TODO(Josh) Test entities for rotation stuff
	Apollo::EntityID mEntity0;
	Apollo::EntityID mEntity1;
};