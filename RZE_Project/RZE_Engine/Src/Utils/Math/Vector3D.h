#pragma once

#include <GLM/vec3.hpp>

class Vector3D
{
public:

	Vector3D();
	Vector3D(const float x, const float y, const float z);
    Vector3D(const int x, const int y, const int z);

	float X() const;
	float Y() const;
	float Z() const;

    void SetX(float newX);
    void SetY(float newY);
    void SetZ(float newY);

    void Normalize();

	const glm::vec3& GetInternalVec() const;

	Vector3D operator+(const Vector3D& rhs) const;
    void operator+=(const Vector3D& rhs);

    Vector3D operator-(Vector3D& rhs) const;

    Vector3D operator*(const Vector3D& rhs) const;
    Vector3D operator*(float scalar) const;

private:
	glm::vec3 mVec;
};
