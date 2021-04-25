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

	const glm::vec3& GetInternalVec() const;

	inline std::string ToString() const
	{
		return std::move(StringUtils::FormatString("[X] %f [Y] %f [Z] %f", X(), Y(), Z()));
	}

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

	bool operator==(const Vector3D& rhs) const;
	bool operator!=(const Vector3D& rhs) const;

	float operator[](int index) const;

private:
	Vector3D(const glm::vec3& vec);

private:
	glm::vec3 mVec;
};
