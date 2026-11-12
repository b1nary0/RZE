#pragma once

#include <Graphics/DynamicVertexBuffer.h>
#include <Graphics/GraphicsLayouts.h>

#include <Rendering/Driver/ShaderTypes.h>

#include <Utils/PrimitiveDefs.h>

#include <cstddef>
#include <unordered_map>
#include <vector>

class MeshGeometry;
struct MeshDraw;

// A pass's mesh draws as instanced draws (Docs/Plans/DrawBatching.md): each draw's transform is an instance in one
// stream, uploaded once a pass, and draws that would bind the same things are one batch, drawn with one call and
// Vertex_MeshInstanced. Each stage that draws meshes owns one. The stage binds what each batch is drawn with; this only
// knows the instances and which draws can share a batch.
//
// A frame: Clear, Add each draw, Upload, BindInstances, then draw each of GetBatches with
// Renderer::DrawIndexedInstanced(indexBuffer, InstanceCount, FirstInstance).
//
// Batches come in the order their first draw was added, and a batch's instances in the order they were added, so the
// draw order is the same every run: exactly coplanar surfaces depend on it (the depth prepass keeps the first, the
// forward pass shades the last).
class MeshBatches
{
public:
	// Draws drawn with one call: the same geometry, and the same bind key
	struct Batch
	{
		// The first draw added to it: the batch's geometry, and what the stage binds for it
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

	// Makes the instance buffer
	void Initialize();

	// Forgets the last pass's draws
	void Clear();
	// Batched with the draws added with the same bindKey and the same geometry. bindKey stands for everything else the
	// stage binds per batch, so draws with equal keys must draw identically; it's only compared, never read. A grouped
	// draw joins its batch wherever that batch is, so it can move ahead of draws added before it, ungrouped ones too.
	// The draw must stay put until the batches are drawn, as a RenderObject's draws do for the frame.
	void Add(const MeshDraw& draw, const void* bindKey);
	// A batch of its own, where it's added, never joined by another: for draws whose order matters (sorted blended
	// draws). Add them after the grouped draws they must be drawn after.
	void AddUngrouped(const MeshDraw& draw);

	// Builds the batches and their instances from what was added, without the GPU
	void Build();
	// Builds, then uploads the instances in one buffer update
	void Upload();

	// Binds the instances where GetInputLayout reads them
	void BindInstances() const;

	// What Build built
	const std::vector<Batch>& GetBatches() const { return m_batches; }
	// Each batch's instances, from its FirstInstance
	const std::vector<GraphicsLayouts::ObjectBufferLayout>& GetInstances() const { return m_instances; }

private:
	// A draw as added
	struct Entry
	{
		const MeshDraw* Draw = nullptr;
		const void* BindKey = nullptr;
		bool IsGrouped = true;
	};

	// What makes two grouped draws one batch
	struct GroupKey
	{
		const void* BindKey = nullptr;
		const MeshGeometry* Geometry = nullptr;

		bool operator==(const GroupKey& other) const { return BindKey == other.BindKey && Geometry == other.Geometry; }
	};

	struct GroupKeyHash
	{
		size_t operator()(const GroupKey& key) const;
	};

private:
	std::vector<Entry> m_entries;

	// The rest are worked out by Build, and kept to reuse their memory
	std::vector<GraphicsLayouts::ObjectBufferLayout> m_instances;
	std::vector<Batch> m_batches;
	// Each entry's batch
	std::vector<U32> m_entryBatches;
	// Each group's batch while building
	std::unordered_map<GroupKey, U32, GroupKeyHash> m_groupBatches;

	DynamicVertexBuffer m_instanceBuffer;
};
