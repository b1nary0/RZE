#pragma once

#include <Utils/Math/Vector3D.h>

#include <array>

class Matrix4x4;

// An axis-aligned box. One made with no arguments is empty: it holds nothing, and growing it by a point or another box
// makes it exactly that. Corner i takes the maximum on X if bit 0 of i is set, on Y if bit 1 is, and on Z if bit 2 is.
class BoundingBox
{
public:
	using Corners = std::array<Vector3D, 8>;

public:
	// Empty
	BoundingBox();
	BoundingBox(const Vector3D& min, const Vector3D& max);

	// The box around points
	static BoundingBox Around(const Corners& points);

public:
	// Whether it holds nothing yet. An empty box's min and max, and everything worked out from them, mean nothing.
	bool IsEmpty() const;

	const Vector3D& GetMin() const { return m_min; }
	const Vector3D& GetMax() const { return m_max; }
	Vector3D GetCentre() const;
	Vector3D GetSize() const;
	// Half its diagonal: the radius of the sphere through its corners
	float GetRadius() const;

	Corners GetCorners() const;
	// Its corners once transformed, e.g. from an object's own space to the world
	Corners GetTransformedCorners(const Matrix4x4& transform) const;
	// The axis-aligned box around it once transformed (the same as the box around its transformed corners, but
	// cheaper); empty for an empty box. For affine transforms (move, rotate, scale).
	BoundingBox Transformed(const Matrix4x4& transform) const;

	void Grow(const Vector3D& point);
	// Growing by an empty box changes nothing
	void Grow(const BoundingBox& box);

	bool operator==(const BoundingBox& other) const;
	bool operator!=(const BoundingBox& other) const { return !(*this == other); }

private:
	Vector3D m_min;
	Vector3D m_max;
};
