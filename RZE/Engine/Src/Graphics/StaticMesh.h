#pragma once

#include <Graphics/MeshGeometry.h>
#include <Graphics/VertexBuffer.h>

#include <Utils/PrimitiveDefs.h>

class StaticMesh
{
public:
	StaticMesh();
	~StaticMesh();

	void Initialize(const std::vector<MeshGeometry>&& meshGeometry);

	std::vector<MeshGeometry>& GetSubMeshes();
	const std::vector<MeshGeometry>& GetSubMeshes() const;

private:
	VertexBuffer m_vertexBuffer;
	std::vector<MeshGeometry> m_subMeshes;
};