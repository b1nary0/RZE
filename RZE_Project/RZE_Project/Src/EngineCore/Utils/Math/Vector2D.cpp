#include "Vector2D.h"

Vector2D::Vector2D()
	: mX(0.0f)
	, mY(0.0f)
{
}

Vector2D::Vector2D(const float x, const float y)
	: mX(x)
	, mY(y)
{
}

float Vector2D::X() const
{
	return mX;
}

float Vector2D::Y() const
{
	return mY;
}
