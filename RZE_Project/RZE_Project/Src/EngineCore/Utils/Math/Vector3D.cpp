#include "Vector3D.h"

Vector3D::Vector3D()
	: mX(0.0f)
	, mY(0.0f)
	, mZ(0.0f)
{
}

Vector3D::Vector3D(const float x, const float y, const float z)
	: mX(x)
	, mY(y)
	, mZ(z)
{
}

float Vector3D::X() const
{
	return mX;
}

float Vector3D::Y() const
{
	return mY;
}

float Vector3D::Z() const
{
	return mZ;
}
