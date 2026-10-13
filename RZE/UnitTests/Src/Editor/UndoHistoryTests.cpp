#include <doctest/doctest.h>

#include <RZE.h>

#include <Services/UndoHistory.h>
#include <UI/Framework/EditorContext.h>

#include <memory>
#include <string>
#include <vector>

using namespace Editor;

namespace
{
	// Logs each undo and redo instead of changing a scene. Records for the same target merge, like a transform
	// record for the same object.
	class FakeRecord : public UndoRecord
	{
	public:
		FakeRecord(std::vector<std::string>& log, std::string name, int target, int before, int after)
			: m_log(log)
			, m_name(std::move(name))
			, m_target(target)
			, m_before(before)
			, m_after(after)
		{
		}

		void Undo(EditorContext& context) override { Log("undo", m_before); }
		void Redo(EditorContext& context) override { Log("redo", m_after); }
		std::string GetName() const override { return m_name; }

		void OnDropped() override { m_log.push_back("dropped " + m_name); }

		bool TryMerge(const UndoRecord& newer) override
		{
			const FakeRecord* const newerFake = dynamic_cast<const FakeRecord*>(&newer);
			if (newerFake == nullptr || newerFake->m_target != m_target)
			{
				return false;
			}

			m_after = newerFake->m_after;
			return true;
		}

	private:
		void Log(const char* action, int value)
		{
			m_log.push_back(std::string(action) + " " + std::to_string(m_target) + "=" + std::to_string(value));
		}

	private:
		std::vector<std::string>& m_log;
		std::string m_name;
		int m_target;
		int m_before;
		int m_after;
	};

	struct UndoHistoryFixture
	{
		EditorContext Context;
		UndoHistory History;
		std::vector<std::string> Log;

		void Record(InteractionID interaction, int target, int before, int after, const char* name = "Change")
		{
			History.Record(interaction, std::make_unique<FakeRecord>(Log, name, target, before, after));
		}
	};

	constexpr InteractionID k_drag = 5;
	constexpr InteractionID k_otherDrag = 6;
}

TEST_CASE_FIXTURE(UndoHistoryFixture, "An empty history has nothing to undo or redo")
{
	CHECK_FALSE(History.CanUndo());
	CHECK_FALSE(History.CanRedo());
	CHECK(History.GetUndoName().empty());
	CHECK(History.GetRedoName().empty());
	CHECK_FALSE(History.Undo(Context));
	CHECK_FALSE(History.Redo(Context));
	CHECK(Log.empty());
}

TEST_CASE_FIXTURE(UndoHistoryFixture, "Undo and redo replay a step and move between the two lists")
{
	Record(k_noInteraction, 1, 10, 20, "Rename");

	CHECK(History.CanUndo());
	CHECK(History.GetUndoName() == "Rename");

	CHECK(History.Undo(Context));
	CHECK(Log == std::vector<std::string>{ "undo 1=10" });
	CHECK_FALSE(History.CanUndo());
	CHECK(History.GetRedoName() == "Rename");

	CHECK(History.Redo(Context));
	CHECK(Log == std::vector<std::string>{ "undo 1=10", "redo 1=20" });
	CHECK(History.CanUndo());
	CHECK_FALSE(History.CanRedo());
}

TEST_CASE_FIXTURE(UndoHistoryFixture, "Changes outside an interaction are a step each")
{
	Record(k_noInteraction, 1, 0, 1);
	Record(k_noInteraction, 1, 1, 2);

	History.Undo(Context);
	CHECK(Log == std::vector<std::string>{ "undo 1=1" });
	CHECK(History.CanUndo());
}

TEST_CASE_FIXTURE(UndoHistoryFixture, "One interaction is one step: undo goes newest first, redo oldest first")
{
	Record(k_drag, 1, 0, 1);
	Record(k_drag, 2, 0, 1);

	History.Undo(Context);
	CHECK(Log == std::vector<std::string>{ "undo 2=0", "undo 1=0" });
	CHECK_FALSE(History.CanUndo());

	History.Redo(Context);
	CHECK(Log == std::vector<std::string>{ "undo 2=0", "undo 1=0", "redo 1=1", "redo 2=1" });
}

TEST_CASE_FIXTURE(UndoHistoryFixture, "Records for the same target in one interaction merge to the first before and last after")
{
	// A drag over two objects, three frames long
	Record(k_drag, 1, 0, 1);
	Record(k_drag, 2, 50, 51);
	Record(k_drag, 1, 1, 2);
	Record(k_drag, 2, 51, 52);
	Record(k_drag, 1, 2, 3);

	History.Undo(Context);
	CHECK(Log == std::vector<std::string>{ "undo 2=50", "undo 1=0" });

	Log.clear();
	History.Redo(Context);
	CHECK(Log == std::vector<std::string>{ "redo 1=3", "redo 2=52" });
}

TEST_CASE_FIXTURE(UndoHistoryFixture, "A different interaction starts a new step")
{
	Record(k_drag, 1, 0, 1);
	Record(k_otherDrag, 1, 1, 2);

	History.Undo(Context);
	CHECK(Log == std::vector<std::string>{ "undo 1=1" });
	CHECK(History.CanUndo());
}

TEST_CASE_FIXTURE(UndoHistoryFixture, "A closed step isn't joined, even by its own interaction")
{
	Record(k_drag, 1, 0, 1);
	History.CloseStep();
	Record(k_drag, 1, 1, 2);

	History.Undo(Context);
	CHECK(Log == std::vector<std::string>{ "undo 1=1" });
	CHECK(History.CanUndo());
}

TEST_CASE_FIXTURE(UndoHistoryFixture, "Undo closes the step, so the same interaction afterwards starts a new one")
{
	Record(k_drag, 1, 0, 1);
	History.Undo(Context);
	History.Redo(Context);
	Record(k_drag, 1, 1, 2);

	Log.clear();
	History.Undo(Context);
	CHECK(Log == std::vector<std::string>{ "undo 1=1" });
}

TEST_CASE_FIXTURE(UndoHistoryFixture, "A new change after an undo drops what could be redone")
{
	Record(k_noInteraction, 1, 0, 1, "First");
	Record(k_noInteraction, 1, 1, 2, "Second");
	History.Undo(Context);
	REQUIRE(History.CanRedo());

	Record(k_noInteraction, 2, 0, 1, "Third");

	CHECK_FALSE(History.CanRedo());
	CHECK(History.GetUndoName() == "Third");
	History.Undo(Context);
	CHECK(History.GetUndoName() == "First");
}

TEST_CASE_FIXTURE(UndoHistoryFixture, "The history keeps the newest k_maxSteps steps")
{
	for (size_t step = 0; step < UndoHistory::k_maxSteps + 1; ++step)
	{
		Record(k_noInteraction, static_cast<int>(step), 0, 1, std::to_string(step).c_str());
	}

	size_t undoCount = 0;
	std::string oldestName;
	while (History.CanUndo())
	{
		oldestName = History.GetUndoName();
		History.Undo(Context);
		++undoCount;
	}

	CHECK(undoCount == UndoHistory::k_maxSteps);
	// Step "0" was dropped
	CHECK(oldestName == "1");
}

TEST_CASE_FIXTURE(UndoHistoryFixture, "Clear empties both lists")
{
	Record(k_noInteraction, 1, 0, 1);
	Record(k_noInteraction, 1, 1, 2);
	History.Undo(Context);

	History.Clear();

	CHECK_FALSE(History.CanUndo());
	CHECK_FALSE(History.CanRedo());

	// The open step went with it
	Record(k_drag, 1, 2, 3);
	Record(k_drag, 2, 2, 3);
	History.Undo(Context);
	CHECK_FALSE(History.CanUndo());
}

TEST_CASE_FIXTURE(UndoHistoryFixture, "Steps past the cap are dropped")
{
	for (size_t step = 0; step < UndoHistory::k_maxSteps + 2; ++step)
	{
		Record(k_noInteraction, static_cast<int>(step), 0, 1, std::to_string(step).c_str());
	}

	CHECK(Log == std::vector<std::string>{ "dropped 0", "dropped 1" });
}

TEST_CASE_FIXTURE(UndoHistoryFixture, "Undone steps are dropped when a new change replaces them")
{
	Record(k_noInteraction, 1, 0, 1, "First");
	Record(k_drag, 2, 0, 1, "Second");
	Record(k_drag, 3, 0, 1, "Second");
	History.Undo(Context);
	Log.clear();

	Record(k_noInteraction, 4, 0, 1, "Third");

	CHECK(Log == std::vector<std::string>{ "dropped Second", "dropped Second" });
}

TEST_CASE_FIXTURE(UndoHistoryFixture, "Clear doesn't drop anything, since the scene is about to unload")
{
	Record(k_noInteraction, 1, 0, 1);
	Record(k_noInteraction, 2, 0, 1);
	History.Undo(Context);

	History.Clear();

	CHECK(Log == std::vector<std::string>{ "undo 2=0" });
}
