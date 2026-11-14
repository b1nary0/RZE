#pragma once

#include <Graphics/MeshBatchGrouping.h>
#include <Graphics/MeshInstanceStream.h>

#include <Utils/PrimitiveDefs.h>

#include <vector>

struct MeshDraw;
struct MeshInstanceFormat;

// A pass's mesh draws as instanced draws (Docs/Plans/DrawBatching.md): draws that would bind the same things are one
// batch (MeshBatchGrouping), drawn with one call from one stream of instances in the pass's format
// (MeshInstanceStream), uploaded once a pass. Each stage that draws meshes owns one. The stage binds what each batch is
// drawn with, and a vertex shader that reads the format (MeshInstanceFormat::InputLayout); this only knows the
// instances and which draws can share a batch.
//
// A frame: Clear, Add each draw, Upload, BindInstances, then draw each of GetBatches with
// Renderer::DrawIndexedInstanced(indexBuffer, InstanceCount, FirstInstance).
class MeshBatches
{
public:
	using Batch = MeshBatchGrouping::Batch;

public:
	// The format must outlive this: a MeshInstanceFormats one
	explicit MeshBatches(const MeshInstanceFormat& format);
	~MeshBatches() = default;

	MeshBatches(const MeshBatches&) = delete;
	MeshBatches& operator=(const MeshBatches&) = delete;

public:
	// Makes the instance buffer
	void Initialize();

	// Forgets the last pass's draws
	void Clear();
	// As MeshBatchGrouping::Add: batched with the draws added with the same bindKey and geometry. For a format without
	// an ID.
	void Add(const MeshDraw& draw, const void* bindKey);
	// The same, for a format that CarriesID: id goes in the draw's instance, so draws of one batch can each have their
	// own (which object they are)
	void Add(const MeshDraw& draw, const void* bindKey, U64 id);
	// As MeshBatchGrouping::AddUngrouped: a batch of its own, where it's added. For a format without an ID.
	void AddUngrouped(const MeshDraw& draw);

	// Builds the batches and their instances, then uploads the instances in one buffer update
	void Upload();

	// Binds the instances where the format's input layout reads them
	void BindInstances() const;

	// What Upload built
	const std::vector<Batch>& GetBatches() const { return m_grouping.GetBatches(); }

private:
	MeshBatchGrouping m_grouping;
	MeshInstanceStream m_instances;
	// The draws' IDs in the order they were added, which is the grouping's placements' order. Empty for a format
	// without one, so the passes that don't use IDs carry none.
	std::vector<U64> m_ids;
};
