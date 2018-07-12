#include <Utils/StdAfx.h>
#include <Utils/Math/Matrix4x4.h>

#include <GLM/gtx/quaternion.hpp>

Matrix4x4::Matrix4x4()
{
	mMat = glm::mat4(1.0f);
}

Matrix4x4::Matrix4x4(const glm::mat4& mat)
{
	mMat = mat;
}

//
// Thanks Phil
//
Matrix4x4 Matrix4x4::CreateInPlace(const Vector3D& position, const Vector3D& scale, const Quaternion& rotation)
{
	const Vector3D axisNormalized = rotation.ToAxis().Normalized();
	const float angle = rotation.ToAngle();

	const float cosine = std::cos(angle);
	const float sine = std::sin(angle);
	const Vector3D temp(axisNormalized * (1.f - cosine));

	Matrix4x4 Matrix;
	Matrix.mMat[0][0] = (cosine + temp.X() * axisNormalized.X()) * scale.X();
	Matrix.mMat[0][1] = 0 + temp.X() * axisNormalized.Y() + sine * axisNormalized.Z();
	Matrix.mMat[0][2] = 0 + temp.X() * axisNormalized.Z() - sine * axisNormalized.Y();
	Matrix.mMat[0][3] = 0;

	Matrix.mMat[1][0] = 0 + temp.Y() * axisNormalized.X() - sine * axisNormalized.Z();
	Matrix.mMat[1][1] = (cosine + temp.Y() * axisNormalized.Y()) * scale.Y();
	Matrix.mMat[1][2] = 0 + temp.Y() * axisNormalized.Z() + sine * axisNormalized.X();
	Matrix.mMat[1][3] = 0;

	Matrix.mMat[2][0] = 0 + temp.Z() * axisNormalized.X() + sine * axisNormalized.Y();
	Matrix.mMat[2][1] = 0 + temp.Z() * axisNormalized.Y() - sine * axisNormalized.X();
	Matrix.mMat[2][2] = (cosine + temp.Z() * axisNormalized.Z()) * scale.Z();
	Matrix.mMat[2][3] = 0;

	Matrix.mMat[3] = glm::vec4(position.GetInternalVec(), 1.0f);

	return Matrix;
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

const Vector3D Matrix4x4::GetPosition() const
{
	return std::move(Vector3D(mMat[3][0], mMat[3][1], mMat[3][2]));
}

const Quaternion Matrix4x4::GetRotation() const
{
	return Quaternion(1.0f);
}

const Vector3D Matrix4x4::GetScale() const
{
	return Vector3D(1.0f);
}

void Matrix4x4::SetPosition(const Vector3D& position)
{
 	mMat[3][0] = position.X();
 	mMat[3][1] = position.Y();
 	mMat[3][2] = position.Z();
 	mMat[3][3] = 1.0f;
}

void Matrix4x4::SetRotation(const Quaternion& rotation)
{
	Rotate(rotation.ToAngle(), rotation.ToAxis());
}

void Matrix4x4::SetScale(const Vector3D& scale)
{
	mMat[0][0] = scale.X();
	mMat[1][1] = scale.Y();
	mMat[2][2] = scale.Z();
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

