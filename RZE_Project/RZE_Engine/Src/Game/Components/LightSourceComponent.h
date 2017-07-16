#pragma once

#include <Game/ECS/EntityComponent.h>

#include <Utils/Math/Vector3D.h>

class LightSourceComponent : public IEntityComponent
{
public:
    const Vector3D& GetPosition() const;
    void SetPosition(const Vector3D& newPosition);

    const Vector3D& GetColor() const;
    void SetColor(const Vector3D& newColor);

    float GetStrength() const;
    void SetStrength(const float newStrength);

private:
    Vector3D mPosition;
    Vector3D mColor;

    float    mStrength;
};