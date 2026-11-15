#include <doctest/doctest.h>

#include <Utils/DebugUtils/DebugConfig.h>

// Render stats only: Retail has no state tracking
#ifdef RZE_RENDER_STATS

#include <Rendering/DrawCounts.h>
#include <Rendering/StateTracker.h>

using namespace Rendering;

namespace
{
	// Objects to bind: only their addresses matter
	int g_first = 0;
	int g_second = 0;

	StateBinding Bound(const void* object, U64 detail = 0)
	{
		StateBinding binding;
		binding.Object = object;
		binding.Detail = detail;
		return binding;
	}
}

TEST_CASE("StateTracker: the first bind of a slot is a change")
{
	StateTracker tracker;

	CHECK_FALSE(tracker.Bind(EStateKind::PixelShader, 0, Bound(&g_first)));
}

TEST_CASE("StateTracker: binding the same object again repeats, another object doesn't")
{
	StateTracker tracker;

	tracker.Bind(EStateKind::TexturePS, 3, Bound(&g_first));
	CHECK(tracker.Bind(EStateKind::TexturePS, 3, Bound(&g_first)));
	CHECK_FALSE(tracker.Bind(EStateKind::TexturePS, 3, Bound(&g_second)));
	CHECK(tracker.Bind(EStateKind::TexturePS, 3, Bound(&g_second)));
}

TEST_CASE("StateTracker: slots and kinds are separate")
{
	StateTracker tracker;

	tracker.Bind(EStateKind::TexturePS, 0, Bound(&g_first));
	CHECK_FALSE(tracker.Bind(EStateKind::TexturePS, 1, Bound(&g_first)));
	CHECK_FALSE(tracker.Bind(EStateKind::TextureCS, 0, Bound(&g_first)));
	CHECK_FALSE(tracker.Bind(EStateKind::ConstantBufferPS, 0, Bound(&g_first)));
	CHECK_FALSE(tracker.Bind(EStateKind::ConstantBufferVS, 0, Bound(&g_first)));
}

TEST_CASE("StateTracker: the detail and the related object are part of the bind")
{
	StateTracker tracker;

	tracker.Bind(EStateKind::UnorderedAccess, 0, Bound(&g_first, 0));
	CHECK_FALSE(tracker.Bind(EStateKind::UnorderedAccess, 0, Bound(&g_first, 1)));

	StateBinding withRelated = Bound(&g_first);
	withRelated.Related = &g_second;
	CHECK_FALSE(tracker.Bind(EStateKind::UnorderedAccess, 0, withRelated));
	CHECK(tracker.Bind(EStateKind::UnorderedAccess, 0, withRelated));
}

TEST_CASE("StateTracker: an unset is a bind of nothing, which repeats once it's unset")
{
	StateTracker tracker;

	tracker.Bind(EStateKind::PixelShader, 0, Bound(&g_first));
	CHECK_FALSE(tracker.Bind(EStateKind::PixelShader, 0, Bound(nullptr)));
	CHECK(tracker.Bind(EStateKind::PixelShader, 0, Bound(nullptr)));
}

TEST_CASE("StateTracker: an unset of a slot nothing was bound to is a change")
{
	StateTracker tracker;

	CHECK_FALSE(tracker.Bind(EStateKind::TexturePS, 5, Bound(nullptr)));
}

TEST_CASE("StateTracker: Clear makes every slot unknown")
{
	StateTracker tracker;
	tracker.Bind(EStateKind::VertexBuffer, 0, Bound(&g_first));
	tracker.Bind(EStateKind::Rasterizer, 0, Bound(nullptr, 2));

	tracker.Clear();

	CHECK_FALSE(tracker.Bind(EStateKind::VertexBuffer, 0, Bound(&g_first)));
	CHECK_FALSE(tracker.Bind(EStateKind::Rasterizer, 0, Bound(nullptr, 2)));
	CHECK(tracker.Bind(EStateKind::VertexBuffer, 0, Bound(&g_first)));
}

TEST_CASE("StateTracker: a new target clears the textures, and nothing else")
{
	StateTracker tracker;
	tracker.Bind(EStateKind::Targets, 0, Bound(&g_first));
	tracker.Bind(EStateKind::TexturePS, 0, Bound(&g_second));
	tracker.Bind(EStateKind::TextureCS, 0, Bound(&g_second));
	tracker.Bind(EStateKind::UnorderedAccess, 0, Bound(&g_second));
	tracker.Bind(EStateKind::ConstantBufferPS, 0, Bound(&g_second));

	CHECK_FALSE(tracker.Bind(EStateKind::Targets, 0, Bound(&g_second)));

	CHECK_FALSE(tracker.Bind(EStateKind::TexturePS, 0, Bound(&g_second)));
	CHECK_FALSE(tracker.Bind(EStateKind::TextureCS, 0, Bound(&g_second)));
	CHECK_FALSE(tracker.Bind(EStateKind::UnorderedAccess, 0, Bound(&g_second)));
	CHECK(tracker.Bind(EStateKind::ConstantBufferPS, 0, Bound(&g_second)));
}

TEST_CASE("StateTracker: binding the same target again keeps the textures")
{
	StateTracker tracker;
	tracker.Bind(EStateKind::Targets, 0, Bound(&g_first));
	tracker.Bind(EStateKind::TexturePS, 0, Bound(&g_second));

	CHECK(tracker.Bind(EStateKind::Targets, 0, Bound(&g_first)));

	CHECK(tracker.Bind(EStateKind::TexturePS, 0, Bound(&g_second)));
}

TEST_CASE("StateTracker: a slot past the tracked ones is never a repeat")
{
	StateTracker tracker;

	CHECK_FALSE(tracker.Bind(EStateKind::ConstantBufferVS, StateTracker::k_slotsPerKind, Bound(&g_first)));
	CHECK_FALSE(tracker.Bind(EStateKind::ConstantBufferVS, StateTracker::k_slotsPerKind, Bound(&g_first)));
}

TEST_CASE("StateTracker: every kind has a category, and every category a name")
{
	for (U32 kind = 0; kind < k_stateKindCount; ++kind)
	{
		CHECK(static_cast<U32>(GetStateCategory(static_cast<EStateKind>(kind))) < k_stateCategoryCount);
	}
	for (U32 category = 0; category < k_stateCategoryCount; ++category)
	{
		CHECK(*GetStateCategoryName(static_cast<EStateCategory>(category)) != '\0');
	}
}

TEST_CASE("StateChangeCounts: draws since an earlier reading take the binds since")
{
	const U32 textures = static_cast<U32>(EStateCategory::Textures);
	const U32 shaders = static_cast<U32>(EStateCategory::Shaders);
	const U32 targets = static_cast<U32>(EStateCategory::Targets);

	DrawCounts earlier;
	earlier.StateChanges.ByCategory[textures].Binds = 10;
	earlier.StateChanges.ByCategory[textures].Repeats = 4;

	DrawCounts later = earlier;
	later.StateChanges.ByCategory[textures].Binds += 7;
	later.StateChanges.ByCategory[textures].Repeats += 3;
	later.StateChanges.ByCategory[shaders].Binds += 2;

	DrawCounts total;
	total += later - earlier;
	total += later - earlier;

	CHECK(total.StateChanges.ByCategory[textures].Binds == 14);
	CHECK(total.StateChanges.ByCategory[textures].Repeats == 6);
	CHECK(total.StateChanges.ByCategory[shaders].Binds == 4);
	CHECK(total.StateChanges.ByCategory[targets].Binds == 0);
}

TEST_CASE("StateChangeCounts: counts become averages, then per frame")
{
	GPUStateChangeCounts counts;
	counts[static_cast<U32>(EStateCategory::Textures)].Binds = 30;
	counts[static_cast<U32>(EStateCategory::Textures)].Repeats = 6;

	const GPUStateChangeAverages totals = ToAverages(counts);
	CHECK(totals[static_cast<U32>(EStateCategory::Textures)].Binds == doctest::Approx(30.0));
	CHECK(totals[static_cast<U32>(EStateCategory::Textures)].Repeats == doctest::Approx(6.0));
	CHECK(totals[static_cast<U32>(EStateCategory::Shaders)].Binds == doctest::Approx(0.0));

	const GPUStateChangeAverages perFrame = ToPerFrame(totals, 4);
	CHECK(perFrame[static_cast<U32>(EStateCategory::Textures)].Binds == doctest::Approx(7.5));
	CHECK(perFrame[static_cast<U32>(EStateCategory::Textures)].Repeats == doctest::Approx(1.5));
}

TEST_CASE("StateChangeCounts: no frames give no per frame figures")
{
	GPUStateChangeAverages totals;
	totals[0].Binds = 12.0;

	CHECK(ToPerFrame(totals, 0)[0].Binds == doctest::Approx(0.0));
	CHECK(ToPerFrame(totals, -1)[0].Binds == doctest::Approx(0.0));
}

#endif // RZE_RENDER_STATS
