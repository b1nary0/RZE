#include <Utils/Math/Vector3D.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/Math/Vector2D.h>
#include <Utils/Math/Matrix4x4.h>

Vector3D Vector3D::ZERO = Vector3D();

Vector3D::Vector3D(const Vector2D& vec2D)
	: mVec(vec2D.X(), vec2D.Y(), 0.0f)
{
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

float Vector3D::Length() const
{
	return glm::length(mVec);
}

Vector3D Vector3D::operator*(const Matrix4x4& rhs) const
{
	glm::vec3 rotVec = glm::vec3(rhs.GetInternalMat() * glm::vec4(mVec, 1.0f));

	return Vector3D(rotVec.x, rotVec.y, rotVec.z);
}

float Vector3D::operator[](int index) const
{
	AssertExpr(index < 3);
	return mVec[index];
}
