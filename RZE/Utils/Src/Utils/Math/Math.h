#pragma once

#include <Math/Quaternion.h>
#include <Math/Vector3D.h>

namespace VectorUtils
{
	Vector3D	Lerp(const Vector3D& from, const Vector3D& to, const float factor);
	float		DistanceSq(const Vector3D& from, const Vector3D& to);
}