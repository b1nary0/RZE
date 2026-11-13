#include <Graphics/MeshInstanceStream.h>

#include <Graphics/MeshInstanceFormat.h>

#include <Rendering/Renderer.h>

#include <Utils/DebugUtils/Debug.h>

MeshInstanceStream::MeshInstanceStream(const MeshInstanceFormat& format)
	: m_format(format)
{
	AssertExpr(m_format.Stride > 0 && m_format.Fill != nullptr);
}

void MeshInstanceStream::Initialize()
{
	m_buffer.Initialize(m_format.Stride);
}

void MeshInstanceStream::Build(const std::vector<MeshBatchGrouping::Placement>& placements)
{
	m_count = static_cast<U32>(placements.size());
	m_instances.resize(static_cast<size_t>(m_count) * m_format.Stride);

	// In the placements' order, the draws': reading the draws in order and writing apart is the faster way round
	std::byte* const instances = m_instances.data();
	for (const MeshBatchGrouping::Placement& placement : placements)
	{
		AssertExpr(placement.Instance < m_count);
		m_format.Fill(*placement.Draw, instances + static_cast<size_t>(placement.Instance) * m_format.Stride);
	}
}

void MeshInstanceStream::Upload()
{
	m_buffer.Upload(m_instances.data(), m_count);
}

void MeshInstanceStream::Bind() const
{
	Rendering::Renderer::SetVertexBuffer(m_buffer.GetPlatformObject(), MeshInstanceFormats::k_instanceSlot);
}

const std::byte* MeshInstanceStream::GetInstance(U32 index) const
{
	AssertExpr(index < m_count);
	return m_instances.data() + static_cast<size_t>(index) * m_format.Stride;
}
