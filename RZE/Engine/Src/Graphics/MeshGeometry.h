#pragma once

#include <memory>

#include <Utils/PrimitiveDefs.h>

#include <Utils/Math/Vector2D.h>
#include <Utils/Math/Vector3D.h>

class Material;
class IndexBuffer;
class VertexBuffer;

struct MeshVertex
{
	Vector3D Position;
	Vector3D Normal;
	Vector3D Tangent;
	Vector2D UVData;
};

class MeshGeometry
{
public:
	MeshGeometry();
	~MeshGeometry();

	void AllocateData();

	void AddVertex(const MeshVertex& vertex);
	void AddIndex(U32 index);

	void SetMaterial(Material* material);

	const Material& GetMaterial() const;
	const std::vector<MeshVertex>& GetVertices();

	const std::vector<float>& GetVertexDataRaw() const;
	const std::vector<U32>& GetIndexDataRaw() const;

private:
	// #TODO
	// Mature this concept, just jotting some ideas down.
	// Essentially the RenderData (or whatever it gets called) needs to be created
	// and essentially "reserved" Renderer-side. Every RenderData node gets allocated
	// an index, vertex, texture, and constant buffer. Data is uploaded to the GPU
	// by using this handle as a key into the Renderer storage to modify or control its lifespan.
	U32 RenderDataHandle;

	std::shared_ptr<VertexBuffer> mVertexBuffer;
	std::shared_ptr<IndexBuffer> mIndexBuffer;

	Material* mMaterial;

	std::vector<MeshVertex> mVertices;
	std::vector<U32> mIndices;
};