#pragma once

#include <Graphics/MeshBatchGrouping.h>
#include <Graphics/MeshInstanceStream.h>

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
	// As MeshBatchGrouping::Add: batched with the draws added with the same bindKey and geometry
	void Add(const MeshDraw& draw, const void* bindKey);
	// As MeshBatchGrouping::AddUngrouped: a batch of its own, where it's added
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
};
