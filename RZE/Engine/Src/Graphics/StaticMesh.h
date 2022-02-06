#pragma once

#include <Graphics/MeshGeometry.h>
#include <Graphics/VertexBuffer.h>

class StaticMesh
{
public:
	StaticMesh();
	~StaticMesh();

	void Initialize(const std::vector<MeshGeometry>&& meshGeometry);

	std::vector<MeshGeometry>& GetSubMeshes();
	const std::vector<MeshGeometry>& GetSubMeshes() const;

private:
	std::vector<MeshGeometry> m_subMeshes;
};