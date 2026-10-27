#pragma once

#include <Utils/Math/Vector3D.h>

#include <vector>

// Fades a debug line out with distance from a point: fully visible within start, gone beyond end.
// Applied per pixel, so a long line fades along its length. end == 0 means no fade.
struct DebugLineFade
{
	Vector3D centre;
	float start = 0.0f;
	float end = 0.0f;
};

struct DebugLine
{
	Vector3D start;
	Vector3D end;
	Vector3D colour;
	// 1 is opaque; lower values blend with what's behind
	float alpha;
	DebugLineFade fade;
	// How opaque it's drawn where it's behind the scene, so e.g. a light's range still shows through the walls it
	// passes behind; 0 hides those parts
	float occludedAlpha = 0.0f;
};

typedef std::vector<DebugLine> DebugLineContainer;
