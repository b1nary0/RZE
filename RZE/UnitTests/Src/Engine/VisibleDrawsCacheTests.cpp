#include <doctest/doctest.h>

#include <Graphics/RenderScene.h>
#include <Graphics/VisibleDrawsCache.h>

#include <Utils/Math/Frustum.h>
#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector3D.h>

#include <vector>

// The scene is empty: splitting what's seen reads each draw's material, and making one needs the renderer. What the
// scene finds is RenderSceneBoundsTests'; these are about when the cache culls. That spans stay valid as results are
// added rests on Get's static_assert (results move, keeping their lists' buffers).

namespace
{
	// Looks down -Z from x metres along the X axis
	CullRequest MakeRequest(float x, ERenderMobilityFilter mobility = ERenderMobilityFilter::Any)
	{
		const Matrix4x4 view = Matrix4x4::CreateViewMatrix(Vector3D(x, 0.0f, 10.0f), Vector3D(x, 0.0f, 0.0f), Vector3D(0.0f, 1.0f, 0.0f));
		return { Frustum(Matrix4x4::CreatePerspectiveMatrixZeroToOne(60.0f, 1.0f, 0.1f, 100.0f) * view), mobility };
	}
}

TEST_CASE("VisibleDrawsCache culls each different request once")
{
	RenderScene scene;
	VisibleDrawsCache cache;

	cache.Get(scene, MakeRequest(0.0f));
	CHECK(cache.GetResultCount() == 1);
	cache.Get(scene, MakeRequest(0.0f));
	CHECK(cache.GetResultCount() == 1);

	// Another frustum, or another mobility, is culled apart
	cache.Get(scene, MakeRequest(1.0f));
	cache.Get(scene, MakeRequest(0.0f, ERenderMobilityFilter::Movable));
	CHECK(cache.GetResultCount() == 3);
	cache.Get(scene, MakeRequest(0.0f));
	CHECK(cache.GetResultCount() == 3);
}

TEST_CASE("VisibleDrawsCache still finds a request after its results have grown")
{
	RenderScene scene;
	VisibleDrawsCache cache;

	cache.Get(scene, MakeRequest(0.0f));
	// Enough for its list of results to grow, and move them, several times
	for (int requestIndex = 1; requestIndex <= 100; ++requestIndex)
	{
		cache.Get(scene, MakeRequest(static_cast<float>(requestIndex)));
	}
	CHECK(cache.GetResultCount() == 101);
	cache.Get(scene, MakeRequest(0.0f));
	CHECK(cache.GetResultCount() == 101);
}

TEST_CASE("VisibleDrawsCache culls again after Reset")
{
	RenderScene scene;
	VisibleDrawsCache cache;

	cache.Get(scene, MakeRequest(0.0f));
	cache.Get(scene, MakeRequest(1.0f));
	cache.Reset();
	CHECK(cache.GetResultCount() == 0);

	cache.Get(scene, MakeRequest(1.0f));
	CHECK(cache.GetResultCount() == 1);
}

// The cull time is render stats, which Retail doesn't have
#ifdef RZE_RENDER_STATS
TEST_CASE("VisibleDrawsCache counts time only while culling")
{
	RenderScene scene;
	VisibleDrawsCache cache;

	cache.Get(scene, MakeRequest(0.0f));
	CHECK(cache.TakeCullSeconds() >= 0.0);
	// Handing back what's culled already isn't culling
	cache.Get(scene, MakeRequest(0.0f));
	CHECK(cache.TakeCullSeconds() == 0.0);
}
#endif
