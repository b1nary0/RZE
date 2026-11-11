#pragma once

#include <Utils/PrimitiveDefs.h>

class BoundingBox;
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

	// Whether a 90-degree view along faceForward (one of the axes, either way) might see something in relativeBox
	// (relative to where it looks from): whether the box reaches into the view's pyramid. Conservative, as the pyramid
	// is tested against the box's extent along each side axis apart.
	bool IsInFaceView(const Vector3D& faceForward, const BoundingBox& relativeBox);
	// The faces of a cube at position that might see something in box (world space), a bit each (1 << ECubeFace); 0
	// for an empty box
	U32 GetFacesSeeing(const Vector3D& position, const BoundingBox& box);
}
