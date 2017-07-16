#include <StdAfx.h>
#include <Game/Components/LightSourceComponent.h>

LightSourceComponent::LightSourceComponent()
{
    SetName("LightSourceComponent");
}

LightSourceComponent::LightSourceComponent(const std::string& name)
{
    SetName(name);
}

LightSourceComponent::~LightSourceComponent()
{
}

const Vector3D& LightSourceComponent::GetPosition() const
{
    return mPosition;
}

void LightSourceComponent::SetPosition(const Vector3D& newPosition)
{
    mPosition = newPosition;
}


const Vector3D& LightSourceComponent::GetColor() const
{
    return mColor;
}

void LightSourceComponent::SetColor(const Vector3D& newColor)
{
    mColor = newColor;
}

float LightSourceComponent::GetStrength() const
{
    return mStrength;
}

void LightSourceComponent::SetStrength(const float newStrength)
{
    mStrength = newStrength;
}
