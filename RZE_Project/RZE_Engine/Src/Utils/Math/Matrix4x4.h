#pragma once

#include <GLM/mat4x4.hpp>

class Vector3D;

class Matrix4x4
{
public:
	Matrix4x4();

	static Matrix4x4 CreateViewMatrix(const Vector3D& eyePos, const Vector3D& centerPos, const Vector3D& upDir);
	static Matrix4x4 CreatePerspectiveMatrix(const float fov, const float aspectRatio, const float nearCull, const float farCull);

	void Translate(const Vector3D& translation);
	void Rotate(const float angle, const Vector3D& axis);
	void Scale(const Vector3D& scale);

	const glm::mat4& GetInternalMat() const;
	const float* GetValuePtr() const;

	Matrix4x4 operator*(const Matrix4x4& rhs) const;

private:
	Matrix4x4(const glm::mat4& mat);

	glm::mat4 mMat;
};
