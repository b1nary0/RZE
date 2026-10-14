#pragma once

#include <Graphics/RenderView.h>

class BoundingBox;
class Vector2D;

namespace Editor
{
	namespace MeshFraming
	{
		// A camera that shows the whole of a world-space box, for a thumbnail: from a three-quarter angle (front-right
		// and a little above), as close as it can be with the box's bounding sphere still inside the view, and with
		// near and far planes just around it
		RenderCamera FrameBounds(const BoundingBox& bounds, float fovYDegrees, const Vector2D& viewportSize);
	}
}
