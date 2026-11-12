#include <Graphics/MeshBatches.h>

#include <Graphics/MeshDraw.h>

#include <Rendering/Renderer.h>

#include <Utils/DebugUtils/Debug.h>

#include <cstddef>

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

void MeshBatches::Build(const std::vector<const MeshDraw*>& draws, std::vector<GraphicsLayouts::ObjectBufferLayout>& outInstances,
	std::vector<Batch>& outBatches)
{
	outInstances.clear();
	outBatches.clear();
	outInstances.reserve(draws.size());
	outBatches.reserve(draws.size());

	for (const MeshDraw* draw : draws)
	{
		// A draw's Transform is the first matrix of an ObjectBufferLayout, its inverse after it (MeshDraw)
		outInstances.push_back(*reinterpret_cast<const GraphicsLayouts::ObjectBufferLayout*>(draw->Transform));

		Batch& batch = outBatches.emplace_back();
		batch.Draw = draw;
		batch.FirstInstance = static_cast<U32>(outInstances.size() - 1);
		batch.InstanceCount = 1;
	}
}

void MeshBatches::Initialize()
{
	m_instanceBuffer.Initialize(sizeof(GraphicsLayouts::ObjectBufferLayout));
}

void MeshBatches::Clear()
{
	m_draws.clear();
}

void MeshBatches::Add(const MeshDraw& draw)
{
	m_draws.push_back(&draw);
}

void MeshBatches::Upload()
{
	Build(m_draws, m_instances, m_batches);
	m_instanceBuffer.Upload(m_instances);
}

void MeshBatches::BindInstances() const
{
	Rendering::Renderer::SetVertexBuffer(m_instanceBuffer.GetPlatformObject(), k_instanceSlot);
}
