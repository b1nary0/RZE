#include <doctest/doctest.h>

#include <UI/TaskTracker.h>

#include <EngineCore/Async/AsyncOperationManager.h>

#include <TestOperation.h>

#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace
{
	// Drives operations and the tracker together, a frame at a time
	struct TrackedTasks
	{
		AsyncOperationManager Manager;
		Editor::TaskTracker Tracker;

		TrackedTasks() { Manager.Initialize(); }
		~TrackedTasks() { Manager.ShutDown(); }

		std::shared_ptr<TestOperation> Start(const std::string& name, U32 flags = EAsyncOperationFlags::None)
		{
			std::shared_ptr<TestOperation> operation = std::make_shared<TestOperation>(name, flags);
			Manager.Start(operation);
			return operation;
		}

		void Frame()
		{
			Manager.Tick();
			Tracker.Update(Manager.GetVisibleOperations());
		}
	};
}

TEST_CASE_FIXTURE(TrackedTasks, "A finished task's note stays until another task finishes")
{
	CHECK(Tracker.GetLastNote() == nullptr);

	std::shared_ptr<TestOperation> task = Start("Rendering thumbnails");
	Frame();
	REQUIRE(Tracker.GetRunning().size() == 1);
	CHECK(Tracker.GetLastNote() == nullptr);

	task->SetStatusText("12 of 12\nSkipped 3 that were up to date");
	task->Succeed();
	Frame();

	CHECK(Tracker.GetRunning().empty());
	REQUIRE(Tracker.GetLastNote() != nullptr);
	CHECK(Tracker.GetLastNote()->Text == "Rendering thumbnails finished: 12 of 12");
	CHECK(Tracker.GetLastNote()->Details == "12 of 12\nSkipped 3 that were up to date");
	CHECK(Tracker.GetLastNote()->Result == EAsyncOperationState::Succeeded);

	for (int frame = 0; frame < 100; ++frame)
	{
		Frame();
	}
	REQUIRE(Tracker.GetLastNote() != nullptr);
	CHECK(Tracker.GetLastNote()->Text == "Rendering thumbnails finished: 12 of 12");

	std::shared_ptr<TestOperation> next = Start("Preloading");
	Frame();
	next->Succeed();
	Frame();
	REQUIRE(Tracker.GetLastNote() != nullptr);
	CHECK(Tracker.GetLastNote()->Text == "Preloading finished");
}

TEST_CASE_FIXTURE(TrackedTasks, "A failed or cancelled task's note says so")
{
	std::shared_ptr<TestOperation> failing = Start("Export");
	Frame();
	failing->FailWith("Disk full\nwhile writing Mesh.png");
	Frame();

	REQUIRE(Tracker.GetLastNote() != nullptr);
	CHECK(Tracker.GetLastNote()->Text == "Export failed: Disk full");
	CHECK(Tracker.GetLastNote()->Name == "Export");
	CHECK(Tracker.GetLastNote()->Details == "Disk full\nwhile writing Mesh.png");
	CHECK(Tracker.GetLastNote()->Result == EAsyncOperationState::Failed);

	std::shared_ptr<TestOperation> cancelled = Start("Import");
	Frame();
	cancelled->RequestCancel();
	Frame();

	REQUIRE(Tracker.GetLastNote() != nullptr);
	CHECK(Tracker.GetLastNote()->Text == "Import cancelled");
	CHECK(Tracker.GetLastNote()->Result == EAsyncOperationState::Cancelled);
}

TEST_CASE_FIXTURE(TrackedTasks, "The last task to finish has the note, and a blocking task is followed like any other")
{
	std::shared_ptr<TestOperation> first = Start("First");
	std::shared_ptr<TestOperation> blocking = Start("Loading Scene", EAsyncOperationFlags::Blocking);
	std::shared_ptr<TestOperation> second = Start("Second");
	Frame();
	CHECK(Tracker.GetRunning().size() == 3);

	first->Succeed();
	Frame();
	blocking->Succeed();
	second->Succeed();
	Frame();

	REQUIRE(Tracker.GetLastNote() != nullptr);
	CHECK(Tracker.GetLastNote()->Text == "Second finished");
}

TEST_CASE_FIXTURE(TrackedTasks, "A task the filter stops showing leaves without an ending, and starts again if it's shown again")
{
	std::shared_ptr<TestOperation> task = Start("Loading Scene", EAsyncOperationFlags::SubStep);
	Frame();
	REQUIRE(Tracker.GetRunning().size() == 1);

	AsyncOperationFilter filter;
	filter.ShowSubSteps = false;
	Manager.SetVisibilityFilter(filter);
	Frame();
	CHECK(Tracker.GetRunning().empty());
	CHECK(Tracker.GetLastNote() == nullptr);

	Manager.SetVisibilityFilter(AsyncOperationFilter());
	Frame();
	CHECK(Tracker.GetRunning().size() == 1);

	size_t startedCount = 0;
	for (const Editor::TaskTracker::HistoryEntry& entry : Tracker.GetHistory())
	{
		startedCount += entry.Kind == Editor::EHistoryKind::Started ? 1 : 0;
	}
	CHECK(startedCount == 2);
}

TEST_CASE_FIXTURE(TrackedTasks, "A task that succeeds with warnings says so")
{
	std::shared_ptr<TestOperation> task = Start("Rendering thumbnails");
	Frame();
	task->AddWarning("Couldn't render SpaceHangar");
	task->AddWarning("Couldn't render Mountain");
	task->Succeed();
	Frame();

	REQUIRE(Tracker.GetLastNote() != nullptr);
	CHECK(Tracker.GetLastNote()->Text == "Rendering thumbnails finished with warnings: Couldn't render SpaceHangar");
	CHECK(Tracker.GetLastNote()->Details == "Couldn't render SpaceHangar\nCouldn't render Mountain");
	CHECK(Tracker.GetLastNote()->Result == EAsyncOperationState::Succeeded);
	CHECK(Tracker.GetLastNote()->HasWarnings);
}

TEST_CASE_FIXTURE(TrackedTasks, "The status is OK until a task runs, then says how the last one went")
{
	CHECK(Tracker.GetStatus() == Editor::ETaskStatus::Ok);

	const auto run = [this](const std::function<void(TestOperation&)>& finish)
	{
		std::shared_ptr<TestOperation> task = Start("Task");
		Frame();
		CHECK(Tracker.GetStatus() == Editor::ETaskStatus::Running);
		finish(*task);
		Frame();
	};

	run([](TestOperation& task) { task.Succeed(); });
	CHECK(Tracker.GetStatus() == Editor::ETaskStatus::Ok);

	run([](TestOperation& task) { task.AddWarning("One item was skipped"); task.Succeed(); });
	CHECK(Tracker.GetStatus() == Editor::ETaskStatus::Warning);

	run([](TestOperation& task) { task.RequestCancel(); });
	CHECK(Tracker.GetStatus() == Editor::ETaskStatus::Warning);

	run([](TestOperation& task) { task.FailWith("Disk full"); });
	CHECK(Tracker.GetStatus() == Editor::ETaskStatus::Failed);

	run([](TestOperation& task) { task.Succeed(); });
	CHECK(Tracker.GetStatus() == Editor::ETaskStatus::Ok);
}

namespace
{
	std::vector<Editor::EHistoryKind> GetKinds(const Editor::TaskTracker& tracker)
	{
		std::vector<Editor::EHistoryKind> kinds;
		for (const Editor::TaskTracker::HistoryEntry& entry : tracker.GetHistory())
		{
			kinds.push_back(entry.Kind);
		}
		return kinds;
	}
}

TEST_CASE_FIXTURE(TrackedTasks, "The history has each task's start, logged steps, warnings and end, in order")
{
	std::shared_ptr<TestOperation> task = Start("Rendering thumbnails");
	Frame();
	task->LogStep("Rendered Sponza");
	task->AddWarning("Couldn't render SpaceHangar");
	Frame();
	// Logged on its last frame, so it comes before the end
	task->LogStep("Rendered Mountain");
	task->Succeed();
	Frame();

	using Kind = Editor::EHistoryKind;
	REQUIRE(GetKinds(Tracker) == std::vector<Kind>{ Kind::Started, Kind::Step, Kind::Warning, Kind::Step, Kind::FinishedWithWarnings });
	const std::deque<Editor::TaskTracker::HistoryEntry>& history = Tracker.GetHistory();
	for (const Editor::TaskTracker::HistoryEntry& entry : history)
	{
		CHECK(entry.Task == "Rendering thumbnails");
	}
	CHECK(history[0].Text == "Started");
	CHECK(history[1].Text == "Rendered Sponza");
	CHECK(history[2].Text == "Couldn't render SpaceHangar");
	CHECK(history[3].Text == "Rendered Mountain");
	CHECK(history[4].Text == "Finished with warnings: Couldn't render SpaceHangar");
}

TEST_CASE_FIXTURE(TrackedTasks, "The history keeps every task, however they ended, and drops the oldest once full")
{
	std::shared_ptr<TestOperation> failing = Start("Export");
	std::shared_ptr<TestOperation> cancelled = Start("Import");
	Frame();
	failing->FailWith("Disk full");
	cancelled->RequestCancel();
	Frame();

	using Kind = Editor::EHistoryKind;
	CHECK(GetKinds(Tracker) == std::vector<Kind>{ Kind::Started, Kind::Started, Kind::Failed, Kind::Cancelled });
	CHECK(Tracker.GetHistory()[2].Task == "Export");
	CHECK(Tracker.GetHistory()[2].Text == "Failed: Disk full");
	CHECK(Tracker.GetHistory()[3].Task == "Import");
	CHECK(Tracker.GetHistory()[3].Text == "Cancelled");

	std::shared_ptr<TestOperation> chatty = Start("Chatty");
	Frame();
	for (size_t step = 0; step < Editor::TaskTracker::k_maxHistoryEntries; ++step)
	{
		chatty->LogStep("Step " + std::to_string(step));
	}
	chatty->Succeed();
	Frame();

	CHECK(Tracker.GetHistory().size() == Editor::TaskTracker::k_maxHistoryEntries);
	CHECK(Tracker.GetHistory().back().Kind == Kind::Finished);
	CHECK(Tracker.GetHistory().back().Text == "Finished");
	// The oldest went: what's left starts partway through Chatty's steps
	CHECK(Tracker.GetHistory().front().Task == "Chatty");
	CHECK(Tracker.GetHistory().front().Kind == Kind::Step);
}

TEST_CASE("Time left is estimated from the rate so far, once a task has gone on long enough to tell")
{
	using Editor::TaskTracker;

	CHECK_FALSE(TaskTracker::EstimateSecondsLeft(10.0, 0.0f).has_value());
	CHECK_FALSE(TaskTracker::EstimateSecondsLeft(10.0, TaskTracker::k_minProgressToEstimate * 0.5f).has_value());
	CHECK_FALSE(TaskTracker::EstimateSecondsLeft(0.5, 0.5f).has_value());

	REQUIRE(TaskTracker::EstimateSecondsLeft(10.0, 0.25f).has_value());
	CHECK(*TaskTracker::EstimateSecondsLeft(10.0, 0.25f) == doctest::Approx(30.0));
	CHECK(*TaskTracker::EstimateSecondsLeft(10.0, 1.0f) == doctest::Approx(0.0));
}

TEST_CASE("Durations are short")
{
	using Editor::TaskTracker;

	CHECK(TaskTracker::FormatDuration(0.2) == "0s");
	CHECK(TaskTracker::FormatDuration(8.4) == "8s");
	CHECK(TaskTracker::FormatDuration(59.4) == "59s");
	CHECK(TaskTracker::FormatDuration(65.0) == "1m 05s");
	CHECK(TaskTracker::FormatDuration(3599.0) == "59m 59s");
	CHECK(TaskTracker::FormatDuration(3600.0 * 2 + 180.0) == "2h 03m");
}

TEST_CASE_FIXTURE(TrackedTasks, "Each run of a task has its own number, so runs at the same time with the same name stay apart")
{
	std::shared_ptr<TestOperation> first = Start("Loading thumbnails");
	std::shared_ptr<TestOperation> second = Start("Loading thumbnails");
	Frame();
	first->LogStep("a.png");
	second->LogStep("b.png");
	first->Succeed();
	Frame();
	second->LogStep("c.png");
	Frame();

	const std::vector<Editor::TaskTracker::HistoryRun> runs = Editor::TaskTracker::GroupByRun(Tracker.GetHistory());
	REQUIRE(runs.size() == 2);
	CHECK(runs[0].Run != runs[1].Run);

	using Kind = Editor::EHistoryKind;
	REQUIRE(runs[0].Entries.size() == 3);
	CHECK(runs[0].Entries[0]->Kind == Kind::Started);
	CHECK(runs[0].Entries[1]->Text == "a.png");
	CHECK(runs[0].Entries[2]->Kind == Kind::Finished);
	CHECK(runs[0].HasEnded());
	CHECK(Tracker.FindRunning(runs[0].Run) == nullptr);

	REQUIRE(runs[1].Entries.size() == 3);
	CHECK(runs[1].Entries[1]->Text == "b.png");
	CHECK(runs[1].Entries[2]->Text == "c.png");
	CHECK_FALSE(runs[1].HasEnded());
	CHECK(Tracker.FindRunning(runs[1].Run) == second.get());

	second->Succeed();
	Frame();
}

TEST_CASE_FIXTURE(TrackedTasks, "A run has problems when it warned, failed or was cancelled")
{
	std::shared_ptr<TestOperation> fine = Start("Fine");
	std::shared_ptr<TestOperation> warned = Start("Warned");
	std::shared_ptr<TestOperation> failed = Start("Failed");
	std::shared_ptr<TestOperation> cancelled = Start("Cancelled");
	Frame();
	fine->Succeed();
	warned->AddWarning("Couldn't render SpaceHangar");
	warned->Succeed();
	failed->FailWith("Disk full");
	cancelled->RequestCancel();
	Frame();

	const std::vector<Editor::TaskTracker::HistoryRun> runs = Editor::TaskTracker::GroupByRun(Tracker.GetHistory());
	REQUIRE(runs.size() == 4);
	CHECK_FALSE(runs[0].HasProblems());
	CHECK(runs[1].HasProblems());
	CHECK(runs[2].HasProblems());
	CHECK(runs[3].HasProblems());
}

TEST_CASE_FIXTURE(TrackedTasks, "Clearing the history forgets it, and a task still running carries on into it")
{
	std::shared_ptr<TestOperation> done = Start("Done");
	std::shared_ptr<TestOperation> running = Start("Running");
	Frame();
	done->Succeed();
	Frame();

	Tracker.ClearHistory();
	CHECK(Tracker.GetHistory().empty());

	running->LogStep("a.png");
	running->Succeed();
	Frame();

	const std::vector<Editor::TaskTracker::HistoryRun> runs = Editor::TaskTracker::GroupByRun(Tracker.GetHistory());
	REQUIRE(runs.size() == 1);
	CHECK(runs[0].Task == "Running");
	REQUIRE(runs[0].Entries.size() == 2);
	CHECK(runs[0].Entries[0]->Text == "a.png");
	CHECK(runs[0].HasEnded());
}
