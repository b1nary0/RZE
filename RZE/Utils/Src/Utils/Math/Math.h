#pragma once

#include <GLM/common.hpp>
#include <GLM/geometric.hpp>

#include <Utils/Math/Quaternion.h>
#include <Utils/Math/Units.h>
#include <Utils/Math/Vector2D.h>
#include <Utils/Math/Vector3D.h>
#include <Utils/PrimitiveDefs.h>

namespace MathUtils
{
	constexpr float Pi = 3.14159265359f;
	constexpr float ToRadians = Pi / 180.0f;
	constexpr float ToDegrees = 180.0f / Pi;

	int Clamp(int value, int min, int max);
	float Clampf(float value, float min, float max);

	// Hermite ease-in/out of t over [0, 1]; t is clamped
	float SmoothStep(float t);

	// The smallest power of two that is >= value (value itself if it already is one)
	U32 CeilPowerOfTwo(U32 value);
}

namespace VectorUtils
{
	constexpr float kEpsilon = 0.00001f;
	constexpr float kEpsilonSq = kEpsilon * kEpsilon;

	float Dot(const Vector3D& a, const Vector3D& b);

	// Component-wise minimum and maximum, e.g. for growing a bounding box
	Vector3D	Min(const Vector3D& a, const Vector3D& b);
	Vector3D	Max(const Vector3D& a, const Vector3D& b);

	Vector3D	Lerp(const Vector3D& from, const Vector3D& to, const float factor);
	Vector2D	Lerp(const Vector2D& from, const Vector2D& to, const float factor);

	// Spherical interpolation between unit vectors, turning at a constant angular rate
	Vector3D	Slerp(const Vector3D& from, const Vector3D& to, const float factor);

	float		DistanceSq(const Vector3D& from, const Vector3D& to);
}

// The one-statement functions, here so callers in other .cpp files can inline them (no build uses LTO)

inline float VectorUtils::Dot(const Vector3D& a, const Vector3D& b)
{
	return glm::dot(a.GetInternalVec(), b.GetInternalVec());
}

inline Vector3D VectorUtils::Min(const Vector3D& a, const Vector3D& b)
{
	const glm::vec3 minimum = glm::min(a.GetInternalVec(), b.GetInternalVec());
	return Vector3D(minimum.x, minimum.y, minimum.z);
}

inline Vector3D VectorUtils::Max(const Vector3D& a, const Vector3D& b)
{
	const glm::vec3 maximum = glm::max(a.GetInternalVec(), b.GetInternalVec());
	return Vector3D(maximum.x, maximum.y, maximum.z);
}

