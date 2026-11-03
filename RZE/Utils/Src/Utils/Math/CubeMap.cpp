#include <StdAfx.h>
#include <Utils/Math/CubeMap.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Units.h>
#include <Utils/Math/Vector3D.h>

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
		case ECubeFace::Count: break;
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
}
