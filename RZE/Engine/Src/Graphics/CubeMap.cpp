#include <StdAfx.h>
#include <Graphics/CubeMap.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/Math/BoundingBox.h>
#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Units.h>
#include <Utils/Math/Vector3D.h>

#include <cmath>

namespace CubeMap
{
	Vector3D GetFaceForward(ECubeFace face)
	{
		switch (face)
		{
		case ECubeFace::Right: return WorldAxes::Right();
		case ECubeFace::Left: return WorldAxes::Right() * -1.0f;
		case ECubeFace::Up: return WorldAxes::Up();
		case ECubeFace::Down: return WorldAxes::Up() * -1.0f;
		case ECubeFace::Forward: return WorldAxes::Forward();
		case ECubeFace::Back: return WorldAxes::Forward() * -1.0f;
		}

		AssertFalse();
		return WorldAxes::Forward();
	}

	Vector3D GetFaceUp(ECubeFace face)
	{
		// The faces around the sides keep the world's up. Looking straight up or down, up in the image is towards the
		// back or forward respectively, which is what D3D's layout gives once Z is flipped.
		switch (face)
		{
		case ECubeFace::Up: return WorldAxes::Forward() * -1.0f;
		case ECubeFace::Down: return WorldAxes::Forward();
		default: return WorldAxes::Up();
		}
	}

	Matrix4x4 CreateFaceView(const Vector3D& position, ECubeFace face)
	{
		return Matrix4x4::CreateViewMatrix(position, position + GetFaceForward(face), GetFaceUp(face));
	}

	Vector3D ToSampleDirection(const Vector3D& worldDirection)
	{
		return Vector3D(worldDirection.X(), worldDirection.Y(), -worldDirection.Z());
	}

	bool IsInFaceView(const Vector3D& faceForward, const BoundingBox& relativeBox)
	{
		int axis = 0;
		for (int candidate = 1; candidate < 3; ++candidate)
		{
			if (std::abs(faceForward[candidate]) > std::abs(faceForward[axis]))
			{
				axis = candidate;
			}
		}

		// How far ahead the box reaches
		const float ahead = faceForward[axis] > 0.0f ? relativeBox.GetMax()[axis] : -relativeBox.GetMin()[axis];
		if (ahead <= 0.0f)
		{
			return false;
		}

		// The pyramid is as wide as it's far ahead
		for (int side = 0; side < 3; ++side)
		{
			if (side != axis && (relativeBox.GetMin()[side] > ahead || relativeBox.GetMax()[side] < -ahead))
			{
				return false;
			}
		}
		return true;
	}

	U32 GetFacesSeeing(const Vector3D& position, const BoundingBox& box)
	{
		if (box.IsEmpty())
		{
			return 0;
		}

		const BoundingBox relativeBox(box.GetMin() - position, box.GetMax() - position);
		U32 faceMask = 0;
		for (U32 face = 0; face < static_cast<U32>(ECubeFace::Count); ++face)
		{
			if (IsInFaceView(GetFaceForward(static_cast<ECubeFace>(face)), relativeBox))
			{
				faceMask |= 1u << face;
			}
		}
		return faceMask;
	}
}
