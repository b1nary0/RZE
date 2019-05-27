#include <Utils/StdAfx.h>
#include <Utils/Math/Matrix4x4.h>

#include <GLM/gtx/quaternion.hpp>
#include <GLM/gtx/matrix_decompose.hpp>

#include <Utils/Math/Math.h>

Matrix4x4::Matrix4x4()
{
	mMat = glm::mat4(1.0f);
}

Matrix4x4::Matrix4x4(const glm::mat4& mat)
{
	mMat = mat;
}

Matrix4x4 Matrix4x4::CreateInPlace(const Vector3D& position, const Vector3D& scale, const Vector3D& rotation)
{
	Quaternion quatRot(rotation);
	glm::mat4 matrix = glm::translate(glm::mat4(), position.GetInternalVec());
	matrix = glm::rotate(matrix, quatRot.ToAngle(), quatRot.ToAxis().GetInternalVec());
	matrix = glm::scale(matrix, scale.GetInternalVec());

	return Matrix4x4(matrix);
}

Matrix4x4 Matrix4x4::CreateViewMatrix(const Vector3D& eyePos, const Vector3D& centerPos, const Vector3D& upDir)
{
	return Matrix4x4(glm::lookAt(eyePos.GetInternalVec(), centerPos.GetInternalVec(), upDir.GetInternalVec()));
}

Matrix4x4 Matrix4x4::CreatePerspectiveMatrix(const float fov, const float aspectRatio, const float nearCull, const float farCull)
{
	return Matrix4x4(glm::perspective(fov * MathUtils::ToRadians, aspectRatio, nearCull, farCull));
}

Matrix4x4 Matrix4x4::CreateOrthoMatrix(const float left, const float right, const float bottom, const float top, const float zNear, const float zFar)
{
	return Matrix4x4(glm::ortho(left, right, bottom, top, zNear, zFar));
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

Matrix4x4 Matrix4x4::Inverse() const
{
	return Matrix4x4(glm::inverse(mMat));
}

const Vector3D Matrix4x4::GetPosition() const
{
	glm::vec3 scale;
	glm::quat rotation;
	glm::vec3 translation;
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(GetInternalMat(), scale, rotation, translation, skew, perspective);

	return Vector3D(translation.x, translation.y, translation.z);
}


const Quaternion Matrix4x4::GetRotation() const
{
	glm::vec3 scale;
	glm::quat rotation;
	glm::vec3 translation;
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(GetInternalMat(), scale, rotation, translation, skew, perspective);

	return Quaternion(rotation.x, rotation.y, rotation.z, rotation.w);
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

Vector4D Matrix4x4::operator*(const Vector4D& rhs) const
{
	glm::vec4 vec = GetInternalMat() * rhs.GetInternalVec();
	return Vector4D(vec.x, vec.y, vec.z, vec.w);
}

