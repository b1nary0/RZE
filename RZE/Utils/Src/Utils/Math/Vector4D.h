#pragma once

#include <GLM/vec4.hpp>

class Vector4D
{
public:
	Vector4D();
	Vector4D(const float x, const float y, const float z, const float w);
	Vector4D(const int x, const int y, const int z, const int w);

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

private:
	glm::vec4 mVec;
};
