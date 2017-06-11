#pragma once

#include <GLM/fwd.hpp>

class Vector4D
{
public:
	Vector4D();
	Vector4D(const float x, const float y, const float z, const float a);

	float X() const;
	float Y() const;
	float Z() const;
	float A() const;

	void Set(const float x, const float y, const float z, const float a);

private:
	glm::vec4 mVec;
};
