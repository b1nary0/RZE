#pragma once

#include <GLM/vec4.hpp>

#include <Utils/StringUtils.h>

class Vector3D;

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

	inline std::string ToString() const
	{
		return std::move(StringUtils::FormatString("[X] %f [Y] %f [Z] %f [W] %f", X(), Y(), Z(), W()));
	}

	//
	// Operators
	//
public:
	Vector4D operator+(const Vector4D& rhs) const;
	Vector4D operator-(const Vector4D& rhs) const;

	float operator[](int index) const;
	void operator=(const Vector3D& rhs);

private:
	glm::vec4 mVec;
};
