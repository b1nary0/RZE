#include <Utils/StdAfx.h>
#include <Utils/Math/Math.h>

namespace VectorUtils
{
	Vector3D Lerp(const Vector3D& from, const Vector3D& to, const float time)
	{
		return from * (1.0f - time) + to * time;
	}

	float DistanceSq(const Vector3D& from, const Vector3D& to)
	{
		Vector3D result = to - from;
		return result.LengthSq();
	}
}
