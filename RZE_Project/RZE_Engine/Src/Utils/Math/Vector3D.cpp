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

Vector3D::Vector3D(const int x, const int y, const int z)
    : mVec(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z))
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

void Vector3D::Normalize()
{
    mVec = glm::normalize(mVec);
}

const glm::vec3& Vector3D::GetInternalVec() const
{
	return mVec;
}

Vector3D Vector3D::operator+(const Vector3D& rhs) const
{
	glm::vec3 addVec = mVec + rhs.GetInternalVec();
	return Vector3D(addVec.x, addVec.y, addVec.z);
}

void Vector3D::operator+=(const Vector3D& rhs)
{
    mVec += rhs.GetInternalVec();
}

Vector3D Vector3D::operator-(Vector3D& rhs) const
{
    glm::vec3 subVec = mVec - rhs.GetInternalVec();
    return Vector3D(subVec.x, subVec.y, subVec.z);
}

void Vector3D::operator-=(const Vector3D& rhs)
{
    mVec -= rhs.GetInternalVec();
}

Vector3D Vector3D::operator*(const Vector3D& rhs) const
{
    glm::vec3 multVec = mVec * rhs.GetInternalVec();
    return Vector3D(multVec.x, multVec.y, multVec.z);
}

Vector3D Vector3D::operator*(float scalar) const
{
    glm::vec3 multVec = mVec * scalar;
    return Vector3D(multVec.x, multVec.y, multVec.z);
}

