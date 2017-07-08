#include <StdAfx.h>
#include <Game/Components/LightSourceComponent.h>

const Vector3D& LightSourceComponent::GetPosition() const
{
    return mPosition;
}

void LightSourceComponent::SetPosition(const Vector3D& newPosition)
{
    mPosition = newPosition;
}
 