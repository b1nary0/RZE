#pragma once

#include <Utils/Math/Quaternion.h>
#include <Utils/Math/Vector3D.h>

namespace MathUtils
{
	constexpr float Pi = 3.14159265359f;
	constexpr float ToRadians = Pi / 180.0f;
	constexpr float ToDegrees = 180.0f / Pi;

	int Clamp(int value, int min, int max);
	float Clampf(float value, float min, float max);
}

namespace VectorUtils
{
	constexpr float kEpsilon = 0.00001f;

	Vector3D	Lerp(const Vector3D& from, const Vector3D& to, const float factor);
	float		DistanceSq(const Vector3D& from, const Vector3D& to);
}