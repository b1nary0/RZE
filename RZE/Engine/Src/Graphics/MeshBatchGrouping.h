#pragma once

#include <Utils/PrimitiveDefs.h>

#include <cstddef>
#include <unordered_map>
#include <vector>

class MeshGeometry;
struct MeshDraw;

// Which of a pass's mesh draws can be drawn with one instanced call (Docs/Plans/DrawBatching.md): draws that would
// bind the same things are one batch. Only the grouping: what an instance holds is a MeshInstanceStream's, and the
// two meet in MeshBatches. No GPU.
//
// A pass: Clear, Add each draw, Build, then GetBatches, and GetPlacements for a stream to build the instances from.
//
// Batches of grouped draws are sorted by state, so the batches that bind the same things come together: by order key
// (what the stage changes least often, e.g. the pixel shader), then by bind key. Each key ranks by when its first draw
// was added, and batches of equal keys stay in the order their first draws were added. In-order batches stay where they
// were added, and grouped batches aren't moved across them. A batch's instances come in the order they were added. So
// the draw order is the same every run whenever the draws are added in the same order, without comparing addresses:
// exactly coplanar surfaces depend on it (the depth prepass keeps the first, the forward pass shades the last).
class MeshBatchGrouping
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

	// Where a draw's instance goes among the batches' instances
	struct Placement
	{
		const MeshDraw* Draw = nullptr;
		U32 Instance = 0;
	};

public:
	// Forgets the last pass's draws
	void Clear();
	// Batched with the draws added with the same bindKey and the same geometry. bindKey stands for everything else the
	// stage binds per batch, so draws with equal keys must draw identically. orderKey is what the stage binds that
	// changes least often, such as the pixel shader: batches with the same one are drawn together. It must be the same
	// for draws with equal bindKeys. Both are only compared, never read. A grouped draw joins its batch wherever that
	// batch is, so it can move ahead of draws added before it, in-order ones too. The draw must stay put until the batches
	// are drawn, as a RenderObject's draws do for the frame.
	void Add(const MeshDraw& draw, const void* bindKey, const void* orderKey);
	// Drawn where it's added, for draws whose order matters (blended draws, sorted back to front): never moved ahead of
	// anything added before it. It joins the batch just before it, as its next instance, only if that batch's draw was
	// also added in order, last, with the same bindKey and geometry: an instanced draw runs its instances in order on
	// both APIs, so the order holds. Anything added between two ends their run. Add them after the grouped draws they
	// must be drawn after.
	void AddInOrder(const MeshDraw& draw, const void* bindKey);

	// Groups what was added into batches
	void Build();

	// What Build built
	const std::vector<Batch>& GetBatches() const { return m_batches; }
	// Each draw's instance, in the order the draws were added rather than the instances' order: a stream reads the draws
	// in the order they're laid out in memory (a RenderObject's after another's), and its writes are what land apart
	const std::vector<Placement>& GetPlacements() const { return m_placements; }
	// How many instances the batches have between them
	U32 GetInstanceCount() const { return static_cast<U32>(m_placements.size()); }

private:
	// A draw as added
	struct Entry
	{
		const MeshDraw* Draw = nullptr;
		const void* BindKey = nullptr;
		// Null for an in-order draw
		const void* OrderKey = nullptr;
		// Add's, rather than AddInOrder's
		bool IsGrouped = true;
	};

	// Where a batch sorts among the grouped batches around it: its keys' ranks
	struct BatchRank
	{
		U32 OrderKey = 0;
		U32 BindKey = 0;
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
	// Sorts each run of grouped batches by state (see the class comment), and points the placements at the batches'
	// new places
	void SortGroupedBatches();

private:
	std::vector<Entry> m_entries;

	// The rest are worked out by Build, and kept to reuse their memory
	std::vector<Batch> m_batches;
	std::vector<Placement> m_placements;
	// Each group's batch while building
	std::unordered_map<GroupKey, U32, GroupKeyHash> m_groupBatches;
	// While sorting: each key's rank, by the first batch that has it
	std::unordered_map<const void*, U32> m_orderKeyRanks;
	std::unordered_map<const void*, U32> m_bindKeyRanks;
	// While sorting: each batch's rank, the batches in their new order, and each batch's new place
	std::vector<BatchRank> m_batchRanks;
	std::vector<U32> m_sortedBatchIndices;
	std::vector<U32> m_newBatchIndices;
	std::vector<Batch> m_sortedBatches;
};
