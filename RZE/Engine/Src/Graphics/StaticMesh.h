#pragma once

#include <memory.h>

#include <Graphics/MeshGeometry.h>

#include <Utils/PrimitiveDefs.h>

#include <Utils/Math/Vector2D.h>
#include <Utils/Math/Vector3D.h>

class VertexBuffer;
class IndexBuffer;

class StaticMesh
{
public:
	StaticMesh();
	~StaticMesh();

	void Initialize(const std::vector<float>& meshData, const std::vector<U32>& indexData);

	U32 GetVertexBuffer() const;
	U32 GetIndexBuffer() const;

private:
	std::unique_ptr<VertexBuffer> mVertexBuffer;
	std::unique_ptr<IndexBuffer> mIndexBuffer;
};