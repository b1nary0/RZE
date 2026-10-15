#pragma once

#include <Utils/Math/BoundingBox.h>
#include <Utils/Math/Matrix4x4.h>

class MeshGeometry;

// A world transform and its inverse, side by side: uploaded together as the vertex shaders' ObjectBuffer
struct MatrixMem
{
	Matrix4x4 transform;
	Matrix4x4 invTransform;
};

// One draw of an object's mesh: a sub-mesh where the mesh places it, in the world. Built by
// StaticMeshInstance::BuildDraws; RenderObject keeps them up to date as the object moves or changes mesh.
struct MeshDraw
{
	const MeshGeometry* Geometry = nullptr;
	// Where it's drawn, with its inverse after it in memory (a MatrixMem), so it uploads as the ObjectBuffer. For a
	// sub-mesh at the mesh's origin it's the object's own matrices, shared by every such draw, so comparing pointers
	// tells a stage when it needs to upload again. Otherwise it's OwnMatrices, which means a MeshDraw can't be copied
	// without rebuilding it: keep draws in the vector BuildDraws filled.
	const Matrix4x4* Transform = nullptr;
	// The sub-mesh's object-space bounding box's corners, transformed into world space
	BoundingBox::Corners Corners;
	// Axis-aligned box around Corners; tighter than the object's bounds, for culling draw by draw
	BoundingBox Bounds;
	// Only used for a sub-mesh away from the mesh's origin: the object's transform times the placement's
	MatrixMem OwnMatrices;
};
