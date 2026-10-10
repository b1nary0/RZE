#include <StdAfx.h>
#include <Utils/Math/Math.h>

#include <Utils/Math/Matrix4x4.h>

#include <GLM/gtc/round.hpp>
#include <GLM/gtx/rotate_vector.hpp>

#include <cmath>

namespace VectorUtils
{
	float Dot(const Vector3D& a, const Vector3D& b)
	{
		return glm::dot(a.GetInternalVec(), b.GetInternalVec());
	}

	Vector3D Min(const Vector3D& a, const Vector3D& b)
	{
		const glm::vec3 minVec = glm::min(a.GetInternalVec(), b.GetInternalVec());
		return Vector3D(minVec.x, minVec.y, minVec.z);
	}

	Vector3D Max(const Vector3D& a, const Vector3D& b)
	{
		const glm::vec3 maxVec = glm::max(a.GetInternalVec(), b.GetInternalVec());
		return Vector3D(maxVec.x, maxVec.y, maxVec.z);
	}

	Vector2D Lerp(const Vector2D& from, const Vector2D& to, const float factor)
	{
		Vector3D ret = Lerp(Vector3D(from.X(), from.Y(), 0.0f), Vector3D(to.X(), to.Y(), 0.0f), factor);
		return Vector2D(ret.X(), ret.Y());
	}

	Vector3D Lerp(const Vector3D& from, const Vector3D& to, const float factor)
	{
		return from * (1.0f - factor) + to * factor;
	}

	Vector3D Slerp(const Vector3D& from, const Vector3D& to, const float factor)
	{
		const float cosAngle = Dot(from, to);

		// glm::slerp divides by sin(angle), which vanishes for parallel or opposite vectors
		if (cosAngle > 1.0f - kEpsilon)
		{
			return Lerp(from, to, factor).Normalized();
		}

		if (cosAngle < -1.0f + kEpsilon)
		{
			// Any perpendicular works as the halfway point; cross with whichever axis isn't near-parallel to from
			const Vector3D axis = std::abs(from.Y()) < 0.9f ? Vector3D(0.0f, 1.0f, 0.0f) : Vector3D(1.0f, 0.0f, 0.0f);
			const Vector3D perpendicular = from.Cross(axis).Normalized();
			const float angle = MathUtils::Pi * factor;
			return from * std::cos(angle) + perpendicular * std::sin(angle);
		}

		const glm::vec3 slerpVec = glm::slerp(from.GetInternalVec(), to.GetInternalVec(), factor);
		return Vector3D(slerpVec.x, slerpVec.y, slerpVec.z);
	}

	float DistanceSq(const Vector3D& from, const Vector3D& to)
	{
		Vector3D result = to - from;
		return result.LengthSq();
	}
}

int MathUtils::Clamp(int value, int min, int max)
{
	return (value < min) ? min : (value > max) ? max : value;
}

float MathUtils::Clampf(float value, float min, float max)
{
	return (value < min) ? min : (value > max) ? max : value;
}

float MathUtils::SmoothStep(float t)
{
	return glm::smoothstep(0.0f, 1.0f, t);
}

U32 MathUtils::CeilPowerOfTwo(U32 value)
{
	return glm::ceilPowerOfTwo(value);
}
