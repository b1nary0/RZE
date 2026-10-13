#include <Services/MeshFraming.h>

#include <Utils/Math/Math.h>
#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Units.h>
#include <Utils/Math/Vector2D.h>
#include <Utils/Math/Vector3D.h>

#include <algorithm>
#include <cmath>

namespace
{
	// Where the camera looks from, around the box: degrees right of straight-on from the front (+Z), and above level
	constexpr float k_yawDegrees = 35.0f;
	constexpr float k_pitchDegrees = 20.0f;
	// A little space around the bounding sphere
	constexpr float k_margin = 1.05f;
	// For a box with no size, so there's still something to look at
	constexpr float k_minRadius = 0.5f;
}

namespace Editor
{
	namespace MeshFraming
	{
		RenderCamera FrameBounds(const Vector3D& boundsMin, const Vector3D& boundsMax, float fovYDegrees, const Vector2D& viewportSize)
		{
			const Vector3D centre = (boundsMin + boundsMax) * 0.5f;
			const float radius = std::max(k_minRadius, (boundsMax - boundsMin).Length() * 0.5f);

			// The sphere has to fit the narrower of the two view angles
			const float aspectRatio = viewportSize.X() / viewportSize.Y();
			const float halfFovY = fovYDegrees * 0.5f * MathUtils::ToRadians;
			const float halfFovX = std::atan(std::tan(halfFovY) * aspectRatio);
			const float distance = radius / std::sin(std::min(halfFovX, halfFovY)) * k_margin;

			const float yaw = k_yawDegrees * MathUtils::ToRadians;
			const float pitch = k_pitchDegrees * MathUtils::ToRadians;
			const Vector3D towardEye(std::sin(yaw) * std::cos(pitch), std::sin(pitch), std::cos(yaw) * std::cos(pitch));
			const Vector3D eye = centre + towardEye * distance;

			// The sphere is all between these
			const float nearCull = (distance - radius) * 0.9f;
			const float farCull = (distance + radius) * 1.1f;

			const Matrix4x4 projection = Matrix4x4::CreatePerspectiveMatrixZeroToOne(fovYDegrees, aspectRatio, nearCull, farCull);
			const Matrix4x4 view = Matrix4x4::CreateViewMatrix(eye, centre, WorldAxes::Up());

			RenderCamera camera;
			camera.ClipSpace = projection * view;
			camera.Position = eye;
			camera.Viewport = RenderViewport{ viewportSize };
			return camera;
		}
	}
}
