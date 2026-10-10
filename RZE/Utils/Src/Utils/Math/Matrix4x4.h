#pragma once

#include <GLM/mat4x4.hpp>

#include <Utils/Math/Quaternion.h>
#include <Utils/Math/Vector3D.h>
#include <Utils/Math/Vector4D.h>

class Matrix4x4
{
public:
	Matrix4x4();

	static Matrix4x4 CreateInPlace(const Vector3D& position, const Vector3D& scale, const Vector3D& eulerDegrees);

	static Matrix4x4 CreateViewMatrix(const Vector3D& eyePos, const Vector3D& centerPos, const Vector3D& upDir);
	static Matrix4x4 CreatePerspectiveMatrix(const float fovYDegrees, const float aspectRatio, const float nearCull, const float farCull);
	// As CreatePerspectiveMatrix but outputs D3D's [0, 1] depth range instead of OpenGL's [-1, 1]
	static Matrix4x4 CreatePerspectiveMatrixZeroToOne(const float fovYDegrees, const float aspectRatio, const float nearCull, const float farCull);
	static Matrix4x4 CreateOrthoMatrix(const float left, const float right, const float bottom, const float top, const float zNear, const float zFar);
	// As CreateOrthoMatrix but outputs D3D's [0, 1] depth range instead of OpenGL's [-1, 1]
	static Matrix4x4 CreateOrthoMatrixZeroToOne(const float left, const float right, const float bottom, const float top, const float zNear, const float zFar);

	void Translate(const Vector3D& translation);
	void Rotate(const float angleRadians, const Vector3D& axis);
	void Scale(const Vector3D& scale);

	Matrix4x4 Inverse() const;
	Matrix4x4 Transpose() const;

	Vector3D GetPosition() const;
	Quaternion GetRotation() const;
	Vector3D GetScale() const;

	void SetPosition(const Vector3D& position);
	void SetScale(const Vector3D& scale);

	// Row 0-3 of the matrix as it multiplies a column vector (matrix * vector)
	Vector4D GetRow(int row) const;

	const glm::mat4& GetInternalMat() const;
	const float* GetValuePtr() const;
	float* GetValuePtr();

	bool operator!=(const Matrix4x4& rhs) const;
	Matrix4x4 operator*(const Matrix4x4& rhs) const;
	Vector4D operator*(const Vector4D& rhs) const;

	static Matrix4x4 IDENTITY;

private:
	Matrix4x4(const glm::mat4& mat);

	glm::mat4 m_mat;
};
