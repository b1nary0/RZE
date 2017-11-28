#include <Utils/StdAfx.h>
#include <Utils/Math/Matrix4x4.h>

#include <Utils/Math/Vector3D.h>

Matrix4x4::Matrix4x4()
{
}

Matrix4x4::Matrix4x4(const glm::mat4& mat)
{
	mMat = mat;
}

Matrix4x4 Matrix4x4::CreateViewMatrix(const Vector3D& eyePos, const Vector3D& centerPos, const Vector3D& upDir)
{
	return Matrix4x4(glm::lookAt(eyePos.GetInternalVec(), centerPos.GetInternalVec(), upDir.GetInternalVec()));
}

Matrix4x4 Matrix4x4::CreatePerspectiveMatrix(const float fov, const float aspectRatio, const float nearCull, const float farCull)
{
	return Matrix4x4(glm::perspective(fov, aspectRatio, nearCull, farCull));
}

void Matrix4x4::Translate(const Vector3D& translation)
{
	mMat = glm::translate(mMat, translation.GetInternalVec());
}

void Matrix4x4::Rotate(const float angle, const Vector3D& axis)
{
	mMat = glm::rotate(mMat, angle, axis.GetInternalVec());
}

void Matrix4x4::Scale(const Vector3D& scale)
{
	mMat = glm::scale(mMat, scale.GetInternalVec());
}

const glm::mat4& Matrix4x4::GetInternalMat() const
{
	return mMat;
}

const float* Matrix4x4::GetValuePtr() const
{
	return glm::value_ptr(mMat);
}

Matrix4x4 Matrix4x4::operator*(const Matrix4x4& rhs) const
{
	return Matrix4x4(mMat * rhs.mMat);
}

