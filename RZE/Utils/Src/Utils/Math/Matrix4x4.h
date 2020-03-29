#pragma once

#include <GLM/mat4x4.hpp>

#include <Utils/Math/Quaternion.h>
#include <Utils/Math/Vector3D.h>
#include <Utils/Math/Vector4D.h>

class Matrix4x4
{
public:
	Matrix4x4();

	static Matrix4x4 CreateInPlace(const Vector3D& position, const Vector3D& scale, const Vector3D& rotation);

	static Matrix4x4 CreateViewMatrix(const Vector3D& eyePos, const Vector3D& centerPos, const Vector3D& upDir);
	static Matrix4x4 CreatePerspectiveMatrix(const float fov, const float aspectRatio, const float nearCull, const float farCull);
	static Matrix4x4 CreateOrthoMatrix(const float left, const float right, const float bottom, const float top, const float zNear, const float zFar);

	void Translate(const Vector3D& translation);
	void Rotate(const float angle, const Vector3D& axis);
	void Scale(const Vector3D& scale);

	Matrix4x4 Inverse() const;

	const Vector3D GetPosition() const;
	const Quaternion GetRotation() const;
	const Vector3D GetScale() const;

	void SetPosition(const Vector3D& position);
	void SetRotation(const Quaternion& rotation);
	void SetScale(const Vector3D& scale);

	const glm::mat4& GetInternalMat() const;
	const float* GetValuePtr() const;

	bool operator!=(const Matrix4x4& rhs) const;
	Matrix4x4 operator*(const Matrix4x4& rhs) const;
	Vector4D operator*(const Vector4D& rhs) const;

private:
	Matrix4x4(const glm::mat4& mat);

	glm::mat4 mMat;
};
