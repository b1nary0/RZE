#pragma once

#include <GLM/vec3.hpp>

#include <Utils/StringUtils.h>

class Matrix4x4;
class Vector2D;

class Vector3D
{
public:
	Vector3D();
	Vector3D(const Vector2D& vec2D);
	Vector3D(const float val);
	Vector3D(const float x, const float y, const float z);
	Vector3D(const int x, const int y, const int z);

public:
	static Vector3D ZERO;

	float X() const;
	float Y() const;
	float Z() const;

	void SetX(float newX);
	void SetY(float newY);
	void SetZ(float newY);

	const Vector3D& Normalize();
	Vector3D Normalized() const;
	Vector3D Cross(const Vector3D& other) const;
	float LengthSq();
	float Length() const;

	const glm::vec3& GetInternalVec() const;
	// X, Y, Z as a float[3], for APIs like ImGui and ImGuizmo
	const float* GetValuePtr() const;
	float* GetValuePtr();

	//
	// Operators
	//
public:
	Vector3D operator+(const Vector3D& rhs) const;
	void operator+=(const Vector3D& rhs);

	Vector3D operator-(const Vector3D& rhs) const;
	void operator-=(const Vector3D& rhs);

	Vector3D operator*(const Vector3D& rhs) const;
	Vector3D operator*(const Matrix4x4& rhs) const;
	Vector3D operator*(float scalar) const;
	void operator*=(float scalar);
	void operator*=(const Vector3D& rhs);

	Vector3D operator/(const float scalar) const;
	Vector3D operator/(const Vector3D& other) const;

	bool operator==(const Vector3D& rhs) const;
	bool operator!=(const Vector3D& rhs) const;

	float operator[](int index) const;

private:
	Vector3D(const glm::vec3& vec);

private:
	glm::vec3 mVec;
};

// The one-statement functions, here so callers in other .cpp files can inline them (no build uses LTO)

inline Vector3D::Vector3D()
	: mVec(0.0f, 0.0f, 0.0f)
{
}

inline Vector3D::Vector3D(const float val)
	: mVec(val, val, val)
{
}

inline Vector3D::Vector3D(const float x, const float y, const float z)
	: mVec(x, y, z)
{
}

inline Vector3D::Vector3D(const int x, const int y, const int z)
	: mVec(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z))
{
}

inline Vector3D::Vector3D(const glm::vec3& vec)
	: mVec(vec)
{
}

inline float Vector3D::X() const
{
	return mVec.x;
}

inline float Vector3D::Y() const
{
	return mVec.y;
}

inline float Vector3D::Z() const
{
	return mVec.z;
}

inline void Vector3D::SetX(float newX)
{
	mVec.x = newX;
}

inline void Vector3D::SetY(float newY)
{
	mVec.y = newY;
}

inline void Vector3D::SetZ(float newZ)
{
	mVec.z = newZ;
}

inline float Vector3D::LengthSq()
{
	return mVec.x * mVec.x + mVec.y * mVec.y + mVec.z * mVec.z;
}

inline const glm::vec3& Vector3D::GetInternalVec() const
{
	return mVec;
}

inline const float* Vector3D::GetValuePtr() const
{
	return &mVec.x;
}

inline float* Vector3D::GetValuePtr()
{
	return &mVec.x;
}

inline Vector3D Vector3D::operator+(const Vector3D& rhs) const
{
	return Vector3D(mVec + rhs.mVec);
}

inline void Vector3D::operator+=(const Vector3D& rhs)
{
	mVec += rhs.mVec;
}

inline Vector3D Vector3D::operator-(const Vector3D& rhs) const
{
	return Vector3D(mVec - rhs.mVec);
}

inline void Vector3D::operator-=(const Vector3D& rhs)
{
	mVec -= rhs.mVec;
}

inline Vector3D Vector3D::operator*(const Vector3D& rhs) const
{
	return Vector3D(mVec * rhs.mVec);
}

inline Vector3D Vector3D::operator*(float scalar) const
{
	return Vector3D(mVec * scalar);
}

inline void Vector3D::operator*=(float scalar)
{
	mVec *= scalar;
}

inline void Vector3D::operator*=(const Vector3D& rhs)
{
	mVec *= rhs.mVec;
}

inline Vector3D Vector3D::operator/(const float scalar) const
{
	return Vector3D(mVec / scalar);
}

inline Vector3D Vector3D::operator/(const Vector3D& other) const
{
	return Vector3D(mVec.x / other.mVec.x, mVec.y / other.mVec.y, mVec.z / other.mVec.z);
}

inline bool Vector3D::operator==(const Vector3D& rhs) const
{
	return rhs.mVec == mVec;
}

inline bool Vector3D::operator!=(const Vector3D& rhs) const
{
	return mVec != rhs.mVec;
}
