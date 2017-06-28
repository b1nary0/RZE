#include <StdAfx.h>
#include <Utils/Math/Vector3d.h>

Vector3D::Vector3D()
	: mVec(0.0f, 0.0f, 0.0f)
{
}

Vector3D::Vector3D(const float x, const float y, const float z)
	: mVec(x, y, z)
{
}

float Vector3D::X() const
{
	return mVec.x;
}

float Vector3D::Y() const
{
	return mVec.y;
}

float Vector3D::Z() const
{
	return mVec.z;
}

void Vector3D::SetX(float newX)
{
    mVec.x = newX;
}

void Vector3D::SetY(float newY)
{
    mVec.y = newY;
}

void Vector3D::SetZ(float newZ)
{
    mVec.z = newZ;
}

const glm::vec3& Vector3D::GetInternalVec() const
{
	return mVec;
}

Vector3D Vector3D::operator+(const Vector3D& rhs)
{
	glm::vec3 addVec = mVec + rhs.GetInternalVec();
	return Vector3D(addVec.x, addVec.y, addVec.z);
}
