#pragma once

#include <GLM/mat4x4.hpp>

#include <Utils/Math/Quaternion.h>
#include <Utils/Math/Vector3D.h>
#include <Utils/Math/Vector4D.h>

class Matrix4x4
{
public:
	Matrix4x4();

	// 16 floats in the order GetValuePtr() gives them (column-major), e.g. read back from a file
	static Matrix4x4 FromValuePtr(const float* values);

	static Matrix4x4 CreateInPlace(const Vector3D& position, const Vector3D& scale, const Vector3D& eulerDegrees);

	static Matrix4x4 CreateViewMatrix(const Vector3D& eyePos, const Vector3D& centerPos, const Vector3D& upDir);
	static Matrix4x4 CreatePerspectiveMatrix(const float fovYDegrees, const float aspectRatio, const float nearCull, const float farCull);
	// As CreatePerspectiveMatrix but outputs D3D's [0, 1] depth range instead of OpenGL's [-1, 1]
	static Matrix4x4 CreatePerspectiveMatrixZeroToOne(const float fovYDegrees, const float aspectRatio, const float nearCull, const float farCull);
	static Matrix4x4 CreateOrthoMatrix(const float left, const float right, const float bottom, const float top, const float zNear, const float zFar);
	// As CreateOrthoMatrix but outputs D3D's [0, 1] depth range instead of OpenGL's [-1, 1]
	static Matrix4x4 CreateOrthoMatrixZeroToOne(const float left, const float right, const float bottom, const float top, const float zNear, const float zFar);
	// Stretches the NDC rect from (minX, minY) to (maxX, maxY) to fill all of clip space. Multiplied in front of a
	// projection, it gives one that only covers that part of the view, e.g. for picking or marquee selection.
	static Matrix4x4 CreateSubRectProjection(const float ndcMinX, const float ndcMinY, const float ndcMaxX, const float ndcMaxY);

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

	bool operator==(const Matrix4x4& rhs) const { return !(*this != rhs); }
	bool operator!=(const Matrix4x4& rhs) const;
	Matrix4x4 operator*(const Matrix4x4& rhs) const;
	Vector4D operator*(const Vector4D& rhs) const;

	static Matrix4x4 IDENTITY;

private:
	Matrix4x4(const glm::mat4& mat);

	glm::mat4 m_mat;
};

// The one-statement functions, here so callers in other .cpp files can inline them (no build uses LTO)

inline Matrix4x4::Matrix4x4()
	: m_mat(1.0f)
{
}

inline Matrix4x4::Matrix4x4(const glm::mat4& mat)
	: m_mat(mat)
{
}

inline const glm::mat4& Matrix4x4::GetInternalMat() const
{
	return m_mat;
}

inline const float* Matrix4x4::GetValuePtr() const
{
	return &m_mat[0].x;
}

inline float* Matrix4x4::GetValuePtr()
{
	return &m_mat[0].x;
}

inline Matrix4x4 Matrix4x4::operator*(const Matrix4x4& rhs) const
{
	return Matrix4x4(m_mat * rhs.m_mat);
}

inline Vector4D Matrix4x4::operator*(const Vector4D& rhs) const
{
	const glm::vec4 product = m_mat * rhs.GetInternalVec();
	return Vector4D(product.x, product.y, product.z, product.w);
}
