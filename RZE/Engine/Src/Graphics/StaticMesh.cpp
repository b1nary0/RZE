#include <StdAfx.h>
#include <Graphics/StaticMesh.h>

#include <Graphics/IndexBuffer.h>
#include <Graphics/VertexBuffer.h>

StaticMesh::StaticMesh()
{
}

StaticMesh::~StaticMesh()
{
}

void StaticMesh::Initialize(const std::vector<float>& meshData, const std::vector<U32>& indexData)
{
	mVertexBuffer = std::make_unique<VertexBuffer>(meshData);
	mVertexBuffer->Initialize();

	mIndexBuffer = std::make_unique<IndexBuffer>(indexData);
	mIndexBuffer->Initialize();
}

U32 StaticMesh::GetVertexBuffer() const
{
	return mVertexBuffer->GetGPUBufferIndex();
}

U32 StaticMesh::GetIndexBuffer() const
{
	return mIndexBuffer->GetGPUBufferIndex();
}
