#pragma once

class Vector2D;
class Vector3D;

// Directions folded onto a square: the sphere projected onto an octahedron, whose lower half is unfolded over the
// corners of the upper half's diamond. Every direction gets one point in -1..1 on both axes and neighbouring directions
// stay neighbours, so a small square texture can store something per direction (the probes' distance maps). The probe
// shaders mirror these.
namespace Octahedral
{
	// A unit direction to its point on the square, -1..1 on both axes. +Z is the centre, -Z the four corners.
	Vector2D Encode(const Vector3D& direction);

	// A point on the square, -1..1 on both axes, to its unit direction
	Vector3D Decode(const Vector2D& point);
}
