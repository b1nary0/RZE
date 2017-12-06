#pragma once

#include <Apollo/ECS/EntitySystem.h>

#include <Utils/Math/Vector3D.h>

class RotateSystem : public Apollo::EntitySystem
{
public:
	RotateSystem();
	virtual ~RotateSystem();

	virtual void Initialize();
	virtual void Update(std::vector<Apollo::EntityID>& entities);
	virtual void ShutDown();

private:
	Vector3D mVelocity;
};