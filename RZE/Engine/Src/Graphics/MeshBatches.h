#pragma once

#include <Graphics/DynamicVertexBuffer.h>
#include <Graphics/GraphicsLayouts.h>

#include <Rendering/Driver/ShaderTypes.h>

#include <Utils/PrimitiveDefs.h>

#include <vector>

struct MeshDraw;

// A pass's mesh draws as instanced draws (Docs/Plans/DrawBatching.md): each draw's transform is an instance in one
// stream, uploaded once a pass, and the draws are drawn in batches, a call each, with Vertex_MeshInstanced. Each stage
// that draws meshes owns one. The stage binds what each batch is drawn with; this only knows the instances.
//
// A frame: Clear, Add each draw in the order to draw it, Upload, BindInstances, then draw each of GetBatches with
// Renderer::DrawIndexedInstanced(indexBuffer, InstanceCount, FirstInstance).
//
// For now each draw is a batch of its own, in the order added.
class MeshBatches
{
public:
	// Draws sharing a geometry and what they're drawn with
	struct Batch
	{
		// Whose geometry and material the batch is drawn with
		const MeshDraw* Draw = nullptr;
		U32 FirstInstance = 0;
		U32 InstanceCount = 0;
	};

public:
	MeshBatches() = default;
	~MeshBatches() = default;

	MeshBatches(const MeshBatches&) = delete;
	MeshBatches& operator=(const MeshBatches&) = delete;

public:
	// What a vertex shader drawn with these takes: the mesh vertex in slot 0, the instance (an ObjectBufferLayout) in slot 1
	static const Rendering::ShaderInputLayout& GetInputLayout();

	// The CPU half of Upload, with no GPU: each draw's transform, in order, and the batches drawing them
	static void Build(const std::vector<const MeshDraw*>& draws, std::vector<GraphicsLayouts::ObjectBufferLayout>& outInstances,
		std::vector<Batch>& outBatches);

	// Makes the instance buffer
	void Initialize();

	// Forgets the last pass's draws
	void Clear();
	// Drawn after the draws added before it. It must stay put until the batches are drawn, as a RenderObject's draws do
	// for the frame.
	void Add(const MeshDraw& draw);
	// Builds the batches and uploads their instances, in one buffer update
	void Upload();

	// Binds the instances where GetInputLayout reads them
	void BindInstances() const;

	// What Upload built
	const std::vector<Batch>& GetBatches() const { return m_batches; }

private:
	std::vector<const MeshDraw*> m_draws;
	// Kept to reuse their memory
	std::vector<GraphicsLayouts::ObjectBufferLayout> m_instances;
	std::vector<Batch> m_batches;

	DynamicVertexBuffer m_instanceBuffer;
};
