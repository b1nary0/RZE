#pragma once

#include <Utils/Math/Quaternion.h>
#include <Utils/Math/Vector3D.h>

#include "Game/ECS/EntityComponent.h"

class TransformComponent : public IEntityComponent
{
public:
	TransformComponent();
    TransformComponent(const std::string& name);
    virtual ~TransformComponent();

	Vector3D& GetPosition();
	Vector3D& GetScale();
	Quaternion& GetRotation();

	void SetPosition(const Vector3D& newPosition);
	void SetScale(const Vector3D& newScale);
    void SetScale(float uniformScale);
	void SetRotation(const Quaternion& newRotation);

private:
	Vector3D mPosition;
	Vector3D mScale;
	Quaternion mRotation; // @note:josh for now, maybe quat later?
};
