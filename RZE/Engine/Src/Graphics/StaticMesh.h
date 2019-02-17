#pragma once

#include <memory.h>

#include <Graphics/MeshGeometry.h>

#include <Utils/PrimitiveDefs.h>

#include <Utils/Math/Vector2D.h>
#include <Utils/Math/Vector3D.h>

class VertexBuffer;

class StaticMesh
{
public:
	StaticMesh();
	~StaticMesh();

	void Initialize(const std::vector<float>& meshData);

	void SetMeshes(const std::vector<MeshGeometry>& meshGeometry);
	const std::vector<MeshGeometry>& GetSubMeshes() const;

private:
	// #TODO(Josh::Redundant storage here until the new mesh data buffer implementation is complete)
	std::vector<MeshGeometry> mSubMeshes;

	std::unique_ptr<VertexBuffer> mBuffer;
};