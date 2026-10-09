#pragma once

#include <Graphics/BufferCapacityPolicy.h>

#include <Rendering/BufferHandle.h>

#include <Utils/DebugUtils/Debug.h>

#include <vector>

// A GPU vertex buffer whose contents are rewritten wholesale each use (debug lines, gizmos, particles...).
// Reallocation is driven by BufferCapacityPolicy, so steady-state use issues no GPU allocations.
// Main thread only, like the rest of Rendering::Renderer.
class DynamicVertexBuffer
{
public:
	DynamicVertexBuffer() = default;
	~DynamicVertexBuffer() = default;

	// stride is the size in bytes of one vertex. Allocates the initial (minimum capacity) buffer.
	void Initialize(U32 stride, const BufferCapacitySettings& settings = BufferCapacitySettings());

	// Replaces the contents with count vertices. Reallocates first if the capacity policy says to.
	// count == 0 still counts toward the shrink policy but issues no GPU work.
	void Upload(const void* data, U32 count);

	template <typename TVertex>
	void Upload(const std::vector<TVertex>& vertices);

	// Vertices written by the last Upload()
	U32 GetCount() const { return m_count; }
	U32 GetCapacity() const { return m_capacityPolicy.GetCapacity(); }
	U32 GetStride() const { return m_stride; }

	const Rendering::VertexBufferHandle& GetPlatformObject() const { return m_gpuBuffer; }

private:
	void Allocate();

private:
	// In-flight render commands hold their own handle, so replacing this keeps the old buffer alive until they're processed
	Rendering::VertexBufferHandle m_gpuBuffer;
	BufferCapacityPolicy m_capacityPolicy;
	U32 m_stride = 0;
	U32 m_count = 0;
};

template <typename TVertex>
void DynamicVertexBuffer::Upload(const std::vector<TVertex>& vertices)
{
	AssertExpr(sizeof(TVertex) == m_stride);
	Upload(vertices.data(), static_cast<U32>(vertices.size()));
}
