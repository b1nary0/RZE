#pragma once

#include <GLM/vec4.hpp>

#include <Utils/Math/Vector3D.h>
#include <Utils/StringUtils.h>

class Vector4D
{
public:
	Vector4D();
	Vector4D(const float val);
	Vector4D(const float x, const float y, const float z, const float w);
	Vector4D(const int x, const int y, const int z, const int w);
	Vector4D(const Vector3D& xyz, const float w);

	static Vector4D ZERO;

	float X() const;
	float Y() const;
	float Z() const;
	float W() const;

	void SetX(float newX);
	void SetY(float newY);
	void SetZ(float newZ);
	void SetW(float newW);

	void Set(const float x, const float y, const float z, const float w);

	// The X, Y and Z components, e.g. a transformed point without its w
	Vector3D XYZ() const;

	const glm::vec4& GetInternalVec() const;
	// X, Y, Z, W as a float[4], for APIs like ImGui
	const float* GetValuePtr() const;
	float* GetValuePtr();

	//
	// Operators
	//
public:
	Vector4D operator+(const Vector4D& rhs) const;
	Vector4D operator-(const Vector4D& rhs) const;

	bool operator==(const Vector4D& rhs) const;
	bool operator!=(const Vector4D& rhs) const;

	float operator[](int index) const;
	void operator=(const Vector3D& rhs);

private:
	glm::vec4 mVec;
};

// The one-statement functions, here so callers in other .cpp files can inline them (no build uses LTO)

inline Vector4D::Vector4D()
	: mVec(0.0f, 0.0f, 0.0f, 0.0f)
{
}

inline Vector4D::Vector4D(const float val)
	: mVec(val, val, val, 1.0f)
{
}

inline Vector4D::Vector4D(const float x, const float y, const float z, const float w)
	: mVec(x, y, z, w)
{
}

inline Vector4D::Vector4D(const int x, const int y, const int z, const int w)
	: mVec(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z), static_cast<float>(w))
{
}

inline Vector4D::Vector4D(const Vector3D& xyz, const float w)
	: mVec(xyz.X(), xyz.Y(), xyz.Z(), w)
{
}

inline float Vector4D::X() const
{
	return mVec.x;
}

inline float Vector4D::Y() const
{
	return mVec.y;
}

inline float Vector4D::Z() const
{
	return mVec.z;
}

inline float Vector4D::W() const
{
	return mVec.w;
}

inline void Vector4D::SetX(float newX)
{
	mVec.x = newX;
}

inline void Vector4D::SetY(float newY)
{
	mVec.y = newY;
}

inline void Vector4D::SetZ(float newZ)
{
	mVec.z = newZ;
}

inline void Vector4D::SetW(float newW)
{
	mVec.w = newW;
}

inline void Vector4D::Set(const float x, const float y, const float z, const float w)
{
	mVec = glm::vec4(x, y, z, w);
}

inline Vector3D Vector4D::XYZ() const
{
	return Vector3D(mVec.x, mVec.y, mVec.z);
}

inline const glm::vec4& Vector4D::GetInternalVec() const
{
	return mVec;
}

inline const float* Vector4D::GetValuePtr() const
{
	return &mVec.x;
}

inline float* Vector4D::GetValuePtr()
{
	return &mVec.x;
}

inline Vector4D Vector4D::operator+(const Vector4D& rhs) const
{
	const glm::vec4 sum = mVec + rhs.mVec;
	return Vector4D(sum.x, sum.y, sum.z, sum.w);
}

inline Vector4D Vector4D::operator-(const Vector4D& rhs) const
{
	const glm::vec4 difference = mVec - rhs.mVec;
	return Vector4D(difference.x, difference.y, difference.z, difference.w);
}

inline bool Vector4D::operator==(const Vector4D& rhs) const
{
	return mVec == rhs.mVec;
}

inline bool Vector4D::operator!=(const Vector4D& rhs) const
{
	return mVec != rhs.mVec;
}

inline float Vector4D::operator[](int index) const
{
	return mVec[index];
}

inline void Vector4D::operator=(const Vector3D& rhs)
{
	mVec = glm::vec4(rhs.X(), rhs.Y(), rhs.Z(), 1.0f);
}
