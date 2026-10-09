#include <StdAfx.h>
#include <Graphics/DynamicVertexBuffer.h>

#include <Rendering/Renderer.h>

void DynamicVertexBuffer::Initialize(U32 stride, const BufferCapacitySettings& settings)
{
	AssertExpr(stride > 0);

	m_stride = stride;
	m_capacityPolicy = BufferCapacityPolicy(settings);
	m_count = 0;

	Allocate();
}

void DynamicVertexBuffer::Upload(const void* data, U32 count)
{
	AssertExpr(m_stride > 0); // Initialize() not called

	if (m_capacityPolicy.Update(count))
	{
		Allocate();
	}

	m_count = count;

	if (count > 0)
	{
		AssertNotNull(data);
		Rendering::Renderer::UpdateVertexBuffer(m_gpuBuffer, data, static_cast<size_t>(count) * m_stride);
	}
}

void DynamicVertexBuffer::Allocate()
{
	m_gpuBuffer = Rendering::Renderer::CreateDynamicVertexBuffer(static_cast<size_t>(m_capacityPolicy.GetCapacity()) * m_stride, m_stride);
}
