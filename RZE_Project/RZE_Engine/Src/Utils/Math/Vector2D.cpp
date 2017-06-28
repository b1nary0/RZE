#include "StdAfx.h"

#include "Vector2D.h"

Vector2D::Vector2D()
    : mVec(0.0f)
{
}

Vector2D::Vector2D(const float x, const float y)
    : mVec(glm::vec2(x, y))
{
}

float Vector2D::X() const
{
	return mVec.x;
}

float Vector2D::Y() const
{
	return mVec.y;
}

void Vector2D::SetX(float newX)
{
    mVec.x = newX;
}

void Vector2D::SetY(float newY)
{
    mVec.y = newY;
}

const glm::vec2& Vector2D::GetInernalVec()
{
    return mVec;
}
