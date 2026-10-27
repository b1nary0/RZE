#include <StdAfx.h>
#include <Utils/Math/Octahedral.h>

#include <Utils/Math/Vector2D.h>
#include <Utils/Math/Vector3D.h>

#include <cmath>

namespace
{
	// 1 for zero, so directions on an axis fold onto the square's edge rather than its middle
	float SignNotZero(float value)
	{
		return value >= 0.0f ? 1.0f : -1.0f;
	}
}

namespace Octahedral
{
	Vector2D Encode(const Vector3D& direction)
	{
		// Onto the octahedron |x| + |y| + |z| = 1
		const float l1Norm = std::abs(direction.X()) + std::abs(direction.Y()) + std::abs(direction.Z());
		const float x = direction.X() / l1Norm;
		const float y = direction.Y() / l1Norm;

		if (direction.Z() >= 0.0f)
		{
			return Vector2D(x, y);
		}

		// The lower half folds out over the corners
		return Vector2D((1.0f - std::abs(y)) * SignNotZero(x), (1.0f - std::abs(x)) * SignNotZero(y));
	}

	Vector3D Decode(const Vector2D& point)
	{
		float x = point.X();
		float y = point.Y();
		const float z = 1.0f - std::abs(x) - std::abs(y);

		// Outside the diamond: fold back underneath
		const float fold = std::fmax(-z, 0.0f);
		x += x >= 0.0f ? -fold : fold;
		y += y >= 0.0f ? -fold : fold;

		return Vector3D(x, y, z).Normalized();
	}
}
