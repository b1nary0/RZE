#pragma once

#include <Graphics/GraphicsDefines.h>

#include <Utils/Math/Frustum.h>

#include <cstddef>
#include <vector>

class RenderScene;
struct MeshDraw;

// What a stage asks to see of the scene: the draws at least partly inside Volume, of the render objects of that
// mobility (RenderScene::CollectDrawsIn)
struct CullRequest
{
	Frustum Volume;
	ERenderMobilityFilter Mobility = ERenderMobilityFilter::Any;

	bool operator==(const CullRequest& other) const;
};

// A run of draws that VisibleDrawsCache holds, for a range-for. A pointer and a count rather than the list itself, so
// what holds them can change (an arena, later) without the stages reading them changing.
struct MeshDrawSpan
{
	const MeshDraw* const* Data = nullptr;
	size_t Count = 0;

	const MeshDraw* const* begin() const { return Data; }
	const MeshDraw* const* end() const { return Data + Count; }
	size_t size() const { return Count; }
	bool empty() const { return Count == 0; }
};

// The draws a CullRequest sees, split by whether their material blends, each in the order CollectDrawsIn found them.
// Valid until the cache's next Reset (the next view), whatever is asked for in between. The draws stay put for the
// frame.
struct VisibleDraws
{
	MeshDrawSpan Opaque;
	MeshDrawSpan Blended;
};

// Culls the scene once for each different CullRequest of a view, so stages that ask for the same one (the depth
// prepass and the forward pass) share what it sees. RenderPipeline resets it at the start of each view; stages reach
// it through RenderContext::GetVisibleDraws.
class VisibleDrawsCache
{
public:
	VisibleDrawsCache();
	~VisibleDrawsCache();
	VisibleDrawsCache(const VisibleDrawsCache&) = delete;
	VisibleDrawsCache& operator=(const VisibleDrawsCache&) = delete;

public:
	// Forgets every result, keeping their storage for the next view's
	void Reset();

	// What request sees of scene: culled the first time it's asked for since Reset, the same draws after that
	VisibleDraws Get(const RenderScene& scene, const CullRequest& request);

	// Seconds spent culling since the last call, then starts over
	double TakeCullSeconds();

	// How many different requests have been culled since Reset
	size_t GetResultCount() const { return m_resultCount; }

private:
	struct Result
	{
		CullRequest Request;
		std::vector<const MeshDraw*> Opaque;
		std::vector<const MeshDraw*> Blended;
	};

	static VisibleDraws ViewOf(const Result& result);

private:
	// Side by side. Growing moves the results, but a moved list keeps its buffer, so the spans handed out stay valid.
	// Kept across Reset with their lists' capacity: the first m_resultCount are this view's.
	std::vector<Result> m_results;
	size_t m_resultCount = 0;
	double m_cullSeconds = 0.0;
};
