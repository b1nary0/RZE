#include <StdAfx.h>
#include <Utils/Math/Math.h>

#include <Utils/Math/Matrix4x4.h>

namespace VectorUtils
{
	float Dot(const Vector3D& a, const Vector3D& b)
	{
		return glm::dot(a.GetInternalVec(), b.GetInternalVec());
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
