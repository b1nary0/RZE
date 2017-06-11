#include <StdAfx.h>
#include <Utils/Math/Vector4D.h>

#include <GLM/vec4.hpp>

Vector4D::Vector4D()
	: mVec(0.0f, 0.0f, 0.0f, 0.0f)
{
}

Vector4D::Vector4D(const float x, const float y, const float z, const float a)
	: mVec(x, y, z, a)
{
}

float Vector4D::X() const
{
	return mVec.x;
}

float Vector4D::Y() const
{
	return mVec.y;
}

float Vector4D::Z() const
{
	return mVec.z;
}

float Vector4D::A() const
{
	return mVec.a;
}

void Vector4D::Set(const float x, const float y, const float z, const float a)
{
	mVec.x = x;
	mVec.y = y;
	mVec.z = z;
	mVec.a = a;
}
