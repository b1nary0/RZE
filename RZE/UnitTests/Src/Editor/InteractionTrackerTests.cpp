#include <doctest/doctest.h>

#include <UI/Framework/InteractionTracker.h>

using namespace Editor;

TEST_CASE("An interaction begins, holds one ID, and ends")
{
	InteractionTracker tracker;
	CHECK_FALSE(tracker.IsActive());
	CHECK(tracker.GetID() == k_noInteraction);

	tracker.Update(true);
	CHECK(tracker.HasBegun());
	CHECK_FALSE(tracker.HasEnded());
	const InteractionID id = tracker.GetID();
	CHECK(id != k_noInteraction);

	tracker.Update(true);
	CHECK(tracker.IsActive());
	CHECK_FALSE(tracker.HasBegun());
	CHECK(tracker.GetID() == id);

	tracker.Update(false);
	CHECK(tracker.HasEnded());
	CHECK_FALSE(tracker.IsActive());
	// Still the one that just ended
	CHECK(tracker.GetID() == id);

	tracker.Update(false);
	CHECK_FALSE(tracker.HasEnded());
}

TEST_CASE("Each interaction gets a new ID, unique across trackers")
{
	InteractionTracker first;
	InteractionTracker second;

	first.Update(true);
	second.Update(true);
	CHECK(first.GetID() != second.GetID());

	const InteractionID firstID = first.GetID();
	first.Update(false);
	first.Update(true);
	CHECK(first.GetID() != firstID);
	CHECK(first.GetID() != second.GetID());
}

TEST_CASE("Reset forgets an interaction without reporting it as ended")
{
	InteractionTracker tracker;
	tracker.Update(true);

	tracker.Reset();
	CHECK_FALSE(tracker.IsActive());

	tracker.Update(false);
	CHECK_FALSE(tracker.HasEnded());

	tracker.Update(true);
	CHECK(tracker.HasBegun());
}

TEST_CASE("NewInteractionID never returns k_noInteraction or repeats")
{
	const InteractionID first = NewInteractionID();
	const InteractionID second = NewInteractionID();
	CHECK(first != k_noInteraction);
	CHECK(second != k_noInteraction);
	CHECK(first != second);
}
