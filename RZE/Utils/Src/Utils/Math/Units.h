#pragma once

#include <Utils/Math/Vector3D.h>

// World conventions. Everything spatial in the engine, editor and content assumes these.
//
// Length:      1 unit = 1 metre. Speeds are metres per second.
// Time:        Seconds (RZE().GetDeltaTime()).
// Axes:        Right-handed; +X right, +Y up, -Z forward (an unrotated camera looks down -Z).
// Angles:      Degrees wherever a person sees or authors them: TransformComponent rotation, camera FOV,
//              the inspector, ImGuizmo and scene files. Radians inside math and trig calls.
//              Convert only with MathUtils::ToRadians / MathUtils::ToDegrees.
// Euler order: R = Rz * Ry * Rx, so X is applied first. This is what glm::quat(eulerRadians) builds in
//              Matrix4x4::CreateInPlace, and what ImGuizmo's Recompose/DecomposeMatrixToComponents use.

namespace Units
{
	constexpr float Metre = 1.0f;
	constexpr float Centimetre = 0.01f;
	constexpr float Millimetre = 0.001f;
}

namespace WorldAxes
{
	inline Vector3D Right() { return Vector3D(1.0f, 0.0f, 0.0f); }
	inline Vector3D Up() { return Vector3D(0.0f, 1.0f, 0.0f); }
	inline Vector3D Forward() { return Vector3D(0.0f, 0.0f, -1.0f); }
}
