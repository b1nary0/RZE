#pragma once

#include <Graphics/DynamicVertexBuffer.h>
#include <Graphics/MeshBatchGrouping.h>

#include <Utils/PrimitiveDefs.h>

#include <cstddef>
#include <vector>

struct MeshDraw;
struct MeshInstanceFormat;

// A pass's mesh instances in one format (MeshInstanceFormat), uploaded once a pass into one vertex buffer and read
// beside the mesh's vertices. Built from where MeshBatchGrouping places each draw's instance. Each stage that draws
// meshes owns one, through MeshBatches.
//
// A pass: Build, Upload, Bind, then draw.
class MeshInstanceStream
{
public:
	// The format must outlive the stream: a MeshInstanceFormats one
	explicit MeshInstanceStream(const MeshInstanceFormat& format);
	~MeshInstanceStream() = default;

	MeshInstanceStream(const MeshInstanceStream&) = delete;
	MeshInstanceStream& operator=(const MeshInstanceStream&) = delete;

public:
	// Makes the buffer
	void Initialize();

	// Each placed draw's instance, as it is now, at its place, without the GPU. Every instance must be placed once, as
	// MeshBatchGrouping's are, so there are as many instances as placements.
	void Build(const std::vector<MeshBatchGrouping::Placement>& placements);
	// What Build built, in one buffer update
	void Upload();

	// Binds the instances where the format's input layout reads them
	void Bind() const;

	const MeshInstanceFormat& GetFormat() const { return m_format; }
	// What Build built
	U32 GetCount() const { return m_count; }
	// The index'th instance Build built: the format's Stride bytes
	const std::byte* GetInstance(U32 index) const;

private:
	const MeshInstanceFormat& m_format;

	// Built by Build; kept to reuse its memory
	std::vector<std::byte> m_instances;
	U32 m_count = 0;

	DynamicVertexBuffer m_buffer;
};
