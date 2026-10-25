#pragma once

#include <Utils/PrimitiveDefs.h>

class Matrix4x4;
class Vector3D;

// The six faces of a cube map, in the order a cube texture stores them (its slices 0-5), named by the world direction
// each one looks in (see Utils/Math/Units.h).
//
// D3D's cube faces are laid out for a left-handed world with +Z forward. Ours is right-handed with -Z forward, so the
// cube's +Z face is our Forward, and sampling flips Z (CubeMap::ToSampleDirection). A face rendered with
// CubeMap::CreateFaceView and a 90 degree, square projection then lines up with what sampling reads, with no
// mirroring, so triangles keep their winding.
enum class ECubeFace : U32
{
	Right,   // +X
	Left,    // -X
	Up,      // +Y
	Down,    // -Y
	Forward, // -Z
	Back,    // +Z
	Count
};

namespace CubeMap
{
	// The world direction the face looks in
	Vector3D GetFaceForward(ECubeFace face);
	// The world direction that's up in the face's image
	Vector3D GetFaceUp(ECubeFace face);
	// The view from position through the face. With a 90 degree, square projection, what it renders lines up with
	// the face's slice of a cube texture.
	Matrix4x4 CreateFaceView(const Vector3D& position, ECubeFace face);

	// The direction to sample a cube texture with (TextureCube in HLSL) to read what was rendered towards
	// worldDirection
	Vector3D ToSampleDirection(const Vector3D& worldDirection);
}
