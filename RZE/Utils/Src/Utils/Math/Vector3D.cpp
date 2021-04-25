#include <Utils/StdAfx.h>
#include <Utils/Math/Vector3d.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/Math/Vector2D.h>
#include <Utils/Math/Matrix4x4.h>

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

Vector3D::Vector3D(const float val)
	: mVec(val, val, val)
{
}

Vector3D::Vector3D(const Vector2D& vec2D)
	: mVec(vec2D.X(), vec2D.Y(), 0.0f)
{
}

Vector3D::Vector3D(const glm::vec3& vec)
	: mVec(vec)
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

const Vector3D& Vector3D::Normalize()
{
	mVec = glm::normalize(mVec);
	return *this;
}

Vector3D Vector3D::Normalized() const
{
	return Vector3D(glm::normalize(mVec));
}

Vector3D Vector3D::Cross(const Vector3D& other) const
{
	glm::vec3 crossVec = glm::cross(mVec, other.GetInternalVec());
	return Vector3D(crossVec.x, crossVec.y, crossVec.z);
}

float Vector3D::LengthSq()
{
	return mVec.x * mVec.x + mVec.y * mVec.y + mVec.z * mVec.z;
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

Vector3D Vector3D::operator-(const Vector3D& rhs) const
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

Vector3D Vector3D::operator*(const Matrix4x4& rhs) const
{
	glm::vec3 rotVec = glm::vec3(rhs.GetInternalMat() * glm::vec4(mVec, 1.0f));

	return Vector3D(rotVec.x, rotVec.y, rotVec.z);
}

void Vector3D::operator*=(float scalar)
{
	mVec *= scalar;
}

void Vector3D::operator*=(const Vector3D& rhs)
{
	mVec *= rhs.GetInternalVec();
}

Vector3D Vector3D::operator/(const float scalar) const
{
	glm::vec3 retVec = mVec / scalar;
	return Vector3D(retVec.x, retVec.y, retVec.z);
}

bool Vector3D::operator==(const Vector3D& rhs) const
{
	return rhs.mVec == mVec;
}

float Vector3D::operator[](int index) const
{
	AssertExpr(index < 3);
	return mVec[index];
}

bool Vector3D::operator!=(const Vector3D& rhs) const
{
	return mVec != rhs.mVec;
}

