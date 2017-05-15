#pragma once

#include "Utils/Math/Vector3D.h"

#include "Game/ECS/EntityComponent.h"

class TransformComponent : public IEntityComponent
{
public:
	Vector3D& GetPosition();

private:
	Vector3D mPosition;
};
