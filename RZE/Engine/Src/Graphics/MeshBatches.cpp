#include <Graphics/MeshBatches.h>

#include <Graphics/MeshDraw.h>

#include <Rendering/Renderer.h>

#include <Utils/DebugUtils/Debug.h>

#include <cstddef>
#include <functional>

namespace
{
	// Where GetInputLayout reads the instances, beside the mesh's vertices in slot 0
	constexpr U32 k_instanceSlot = 1;

	// The instance stream's elements are the ObjectBufferLayout's bytes as they are: two matrices of four columns
	static_assert(sizeof(Matrix4x4) == 16 * sizeof(float), "A Matrix4x4 isn't four float4 columns");
	static_assert(offsetof(GraphicsLayouts::ObjectBufferLayout, InvTransform) == sizeof(Matrix4x4),
		"ObjectBufferLayout's matrices aren't side by side");
	static_assert(sizeof(GraphicsLayouts::ObjectBufferLayout) == 2 * sizeof(Matrix4x4), "ObjectBufferLayout isn't only its matrices");
}

const Rendering::ShaderInputLayout& MeshBatches::GetInputLayout()
{
	using Rendering::EDataClassification;
	using Rendering::EDataFormat;

	// Vertex_MeshInstanced's VertexInput (ForwardTypes.hlsli) and MeshInstanceInput, in that order
	static const Rendering::ShaderInputLayout inputLayout =
	{
		{ "POSITION", EDataFormat::R32G32B32_FLOAT, EDataClassification::PER_VERTEX, 0 },
		{ "NORMAL", EDataFormat::R32G32B32_FLOAT, EDataClassification::PER_VERTEX, 12 },
		{ "UV", EDataFormat::R32G32_FLOAT, EDataClassification::PER_VERTEX, 24 },
		{ "TANGENT", EDataFormat::R32G32B32_FLOAT, EDataClassification::PER_VERTEX, 32 },
		{ "WORLDA", EDataFormat::R32G32B32A32_FLOAT, EDataClassification::PER_INSTANCE, 0, k_instanceSlot },
		{ "WORLDB", EDataFormat::R32G32B32A32_FLOAT, EDataClassification::PER_INSTANCE, 16, k_instanceSlot },
		{ "WORLDC", EDataFormat::R32G32B32A32_FLOAT, EDataClassification::PER_INSTANCE, 32, k_instanceSlot },
		{ "WORLDD", EDataFormat::R32G32B32A32_FLOAT, EDataClassification::PER_INSTANCE, 48, k_instanceSlot },
		{ "INVWORLDA", EDataFormat::R32G32B32A32_FLOAT, EDataClassification::PER_INSTANCE, 64, k_instanceSlot },
		{ "INVWORLDB", EDataFormat::R32G32B32A32_FLOAT, EDataClassification::PER_INSTANCE, 80, k_instanceSlot },
		{ "INVWORLDC", EDataFormat::R32G32B32A32_FLOAT, EDataClassification::PER_INSTANCE, 96, k_instanceSlot },
		{ "INVWORLDD", EDataFormat::R32G32B32A32_FLOAT, EDataClassification::PER_INSTANCE, 112, k_instanceSlot }
	};
	return inputLayout;
}

void MeshBatches::Initialize()
{
	m_instanceBuffer.Initialize(sizeof(GraphicsLayouts::ObjectBufferLayout));
}

void MeshBatches::Clear()
{
	m_entries.clear();
}

void MeshBatches::Add(const MeshDraw& draw, const void* bindKey)
{
	m_entries.push_back({ &draw, bindKey, true });
}

void MeshBatches::AddUngrouped(const MeshDraw& draw)
{
	m_entries.push_back({ &draw, nullptr, false });
}

void MeshBatches::Build()
{
	m_instances.clear();
	m_batches.clear();
	m_entryBatches.clear();
	m_groupBatches.clear();
	m_entryBatches.reserve(m_entries.size());

	// Each entry's batch, a batch made where its first entry is, and how many instances each has. A stable counting
	// sort: no keys are compared for order, so the order is the same every run.
	for (const Entry& entry : m_entries)
	{
		U32 batchIndex = static_cast<U32>(m_batches.size());
		if (entry.IsGrouped)
		{
			const auto [it, isNew] = m_groupBatches.try_emplace(GroupKey{ entry.BindKey, entry.Draw->Geometry }, batchIndex);
			batchIndex = it->second;
			if (isNew)
			{
				m_batches.push_back({ entry.Draw, 0, 0 });
			}
		}
		else
		{
			m_batches.push_back({ entry.Draw, 0, 0 });
		}

		++m_batches[batchIndex].InstanceCount;
		m_entryBatches.push_back(batchIndex);
	}

	// Each batch's instances follow the last's. The counts start again from 0, to count the instances placed.
	U32 instanceCount = 0;
	for (Batch& batch : m_batches)
	{
		batch.FirstInstance = instanceCount;
		instanceCount += batch.InstanceCount;
		batch.InstanceCount = 0;
	}

	// Each entry's matrices in the next place in its batch: a draw's Transform is the first matrix of an
	// ObjectBufferLayout, its inverse after it (MeshDraw)
	m_instances.resize(instanceCount);
	for (size_t entryIndex = 0; entryIndex < m_entries.size(); ++entryIndex)
	{
		Batch& batch = m_batches[m_entryBatches[entryIndex]];
		m_instances[batch.FirstInstance + batch.InstanceCount] =
			*reinterpret_cast<const GraphicsLayouts::ObjectBufferLayout*>(m_entries[entryIndex].Draw->Transform);
		++batch.InstanceCount;
	}
}

void MeshBatches::Upload()
{
	Build();
	m_instanceBuffer.Upload(m_instances);
}

void MeshBatches::BindInstances() const
{
	Rendering::Renderer::SetVertexBuffer(m_instanceBuffer.GetPlatformObject(), k_instanceSlot);
}

size_t MeshBatches::GroupKeyHash::operator()(const GroupKey& key) const
{
	const size_t bindKeyHash = std::hash<const void*>()(key.BindKey);
	const size_t geometryHash = std::hash<const void*>()(key.Geometry);
	// Mixes the two, so swapping them, or keys differing in only one, hash differently
	return bindKeyHash ^ (geometryHash + 0x9e3779b97f4a7c15ull + (bindKeyHash << 6) + (bindKeyHash >> 2));
}
