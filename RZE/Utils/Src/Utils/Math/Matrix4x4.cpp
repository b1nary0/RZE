#include <Utils/StdAfx.h>
#include <Utils/Math/Matrix4x4.h>

#include <GLM/gtx/quaternion.hpp>
#include <GLM/gtx/matrix_decompose.hpp>
#include <GLM/gtx/euler_angles.hpp>

#include <Utils/Math/Math.h>

Matrix4x4 Matrix4x4::IDENTITY = Matrix4x4(glm::mat4(1.0f));

Matrix4x4::Matrix4x4()
{
	m_mat = glm::mat4(1.0f);
}

Matrix4x4::Matrix4x4(const glm::mat4& mat)
{
	m_mat = mat;
}

Matrix4x4 Matrix4x4::CreateInPlace(const Vector3D& position, const Vector3D& scale, const Vector3D& rotation)
{
	Quaternion quatRot(rotation * MathUtils::ToRadians);
	glm::mat4 matrix = glm::translate(glm::mat4(1.0f), position.GetInternalVec());
	matrix *= glm::toMat4(quatRot.GetInternalQuat());
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
	m_mat = glm::translate(m_mat, translation.GetInternalVec());
}

void Matrix4x4::Rotate(const float angle, const Vector3D& axis)
{
	m_mat = glm::rotate(m_mat, angle, axis.GetInternalVec());
}

void Matrix4x4::Rotate(const Vector3D& rotation)
{
	Quaternion quatRot = Quaternion(rotation);
	float angle = quatRot.ToAngle();
	Vector3D axis = quatRot.ToAxis();
	
	m_mat = glm::rotate(m_mat, angle, axis.GetInternalVec());
}

void Matrix4x4::Scale(const Vector3D& scale)
{
	m_mat = glm::scale(m_mat, scale.GetInternalVec());
}

Matrix4x4 Matrix4x4::Inverse() const
{
	return Matrix4x4(glm::inverse(m_mat));
}

Matrix4x4 Matrix4x4::Transpose() const
{
	return Matrix4x4(glm::transpose(m_mat));
}

Vector3D Matrix4x4::GetPosition() const
{
	glm::vec3 scale;
	glm::quat rotation;
	glm::vec3 translation;
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(GetInternalMat(), scale, rotation, translation, skew, perspective);

	return Vector3D(translation.x, translation.y, translation.z);
}

Quaternion Matrix4x4::GetRotation() const
{
	glm::vec3 scale;
	glm::quat rotation;
	glm::vec3 translation;
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(GetInternalMat(), scale, rotation, translation, skew, perspective);

	return Quaternion(rotation.x, rotation.y, rotation.z, rotation.w);
}

Vector3D Matrix4x4::GetScale() const
{
	glm::vec3 scale;
	glm::quat rotation;
	glm::vec3 translation;
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(GetInternalMat(), scale, rotation, translation, skew, perspective);

	return Vector3D(scale.x, scale.y, scale.z);
}

void Matrix4x4::SetPosition(const Vector3D& position)
{
	m_mat[3] = Vector4D(position.X(), position.Y(), position.Z(), 1.0f).GetInternalVec();
}

void Matrix4x4::SetRotation(const Vector3D& rotation)
{
	m_mat = m_mat * glm::eulerAngleXYZ(rotation.X(), rotation.Y(), rotation.Z());
}

void Matrix4x4::SetScale(const Vector3D& scale)
{
	m_mat[0].x = scale.X();
	m_mat[1].y = scale.Y();
	m_mat[2].z = scale.Z();
}

const glm::mat4& Matrix4x4::GetInternalMat() const
{
	return m_mat;
}

const float* Matrix4x4::GetValuePtr() const
{
	return glm::value_ptr(m_mat);
}

float* Matrix4x4::GetValuePtr()
{
	return glm::value_ptr(m_mat);
}

bool Matrix4x4::operator!=(const Matrix4x4& rhs) const
{
	glm::mat4 rhsInternalMat = rhs.GetInternalMat();
	bool bEqual = true;
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			if (m_mat[i][j] != rhsInternalMat[i][j])
			{
				bEqual = false;
				break;
			}
		}
	}

	return !bEqual;
}

Matrix4x4 Matrix4x4::operator*(const Matrix4x4& rhs) const
{
	return Matrix4x4(m_mat * rhs.m_mat);
}

Vector4D Matrix4x4::operator*(const Vector4D& rhs) const
{
	glm::vec4 vec = GetInternalMat() * rhs.GetInternalVec();
	return Vector4D(vec.x, vec.y, vec.z, vec.w);
}

