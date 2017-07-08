#pragma once

#include <Game/ECS/EntityComponent.h>

#include <Utils/Math/Vector3D.h>

class LightSourceComponent : public IEntityComponent
{
public:
    const Vector3D& GetPosition() const;
    void SetPosition(const Vector3D& newPosition);

private:
    Vector3D mPosition;
};