#include <Graphics/VisibleDrawsCache.h>

#include <Graphics/Material.h>
#include <Graphics/MaterialSlot.h>
#include <Graphics/MeshDraw.h>
#include <Graphics/RenderScene.h>

#include <Utils/TimeUtils.h>

#include <type_traits>

bool CullRequest::operator==(const CullRequest& other) const
{
	return Mobility == other.Mobility && Volume == other.Volume;
}

VisibleDrawsCache::VisibleDrawsCache() = default;
VisibleDrawsCache::~VisibleDrawsCache() = default;

void VisibleDrawsCache::Reset()
{
	m_resultCount = 0;
}

VisibleDraws VisibleDrawsCache::Get(const RenderScene& scene, const CullRequest& request)
{
	// Otherwise m_results would copy its results as it grows, and the spans handed out would point into freed lists
	static_assert(std::is_nothrow_move_constructible<Result>::value, "Results must move, keeping their lists' buffers");

	// A view asks for a handful (one for the camera, one per light view rendered), so a search is enough
	for (size_t resultIndex = 0; resultIndex < m_resultCount; ++resultIndex)
	{
		if (m_results[resultIndex].Request == request)
		{
			return ViewOf(m_results[resultIndex]);
		}
	}

#ifdef RZE_RENDER_STATS
	const double cullStart = TimeUtils::GetSeconds();
#endif

	if (m_resultCount == m_results.size())
	{
		m_results.push_back(Result{ request, {}, {} });
	}
	Result& result = m_results[m_resultCount];
	++m_resultCount;
	result.Request = request;
	result.Blended.clear();

	// Everything seen goes into Opaque, then the blended draws move out, the rest closing up behind them: one list
	// written, no scratch one
	scene.CollectDrawsIn(request.Volume, request.Mobility, result.Opaque);
	size_t opaqueCount = 0;
	for (size_t drawIndex = 0; drawIndex < result.Opaque.size(); ++drawIndex)
	{
		const MeshDraw* const draw = result.Opaque[drawIndex];
		if (draw->Slot->GetMaterial().IsBlended())
		{
			result.Blended.push_back(draw);
		}
		else
		{
			result.Opaque[opaqueCount] = draw;
			++opaqueCount;
		}
	}
	result.Opaque.resize(opaqueCount);

#ifdef RZE_RENDER_STATS
	m_cullSeconds += TimeUtils::GetSeconds() - cullStart;
#endif
	return ViewOf(result);
}

#ifdef RZE_RENDER_STATS
double VisibleDrawsCache::TakeCullSeconds()
{
	const double cullSeconds = m_cullSeconds;
	m_cullSeconds = 0.0;
	return cullSeconds;
}
#endif

VisibleDraws VisibleDrawsCache::ViewOf(const Result& result)
{
	return { { result.Opaque.data(), result.Opaque.size() }, { result.Blended.data(), result.Blended.size() } };
}
