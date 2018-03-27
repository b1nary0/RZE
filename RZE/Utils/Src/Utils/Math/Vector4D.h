#pragma once

#include <GLM/vec4.hpp>

#include <Utils/StringUtils.h>

class Vector4D
{
public:
	Vector4D();
	Vector4D(const float val);
	Vector4D(const float x, const float y, const float z, const float w);
	Vector4D(const int x, const int y, const int z, const int w);

	inline static Vector4D& Default() { return sDefaultVec; }

	float X() const;
	float Y() const;
	float Z() const;
	float W() const;

	void SetX(float newX);
	void SetY(float newY);
	void SetZ(float newZ);
	void SetW(float newW);

	void Set(const float x, const float y, const float z, const float w);

	const glm::vec4& GetInternalVec() const;

	inline std::string ToString()
	{
		return std::move(StringUtils::FormatString("[X] %f [Y] %f [Z] %f [W] %f", X(), Y(), Z(), W()));
	}

	//
	// Operators
	//
public:

private:
	static Vector4D sDefaultVec;
	glm::vec4 mVec;
};
