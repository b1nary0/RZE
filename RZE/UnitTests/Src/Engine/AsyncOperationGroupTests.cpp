#include <doctest/doctest.h>

#include <EngineCore/Async/AsyncOperationGroup.h>
#include <EngineCore/Async/AsyncOperationManager.h>

#include <TestOperation.h>

#include <memory>
#include <string>

namespace
{
	// A group and the manager that drives it, with its operations started as sub-steps
	struct GroupedTasks
	{
		AsyncOperationManager Manager;
		std::shared_ptr<AsyncOperationGroup> Group;

		GroupedTasks()
		{
			Manager.Initialize();
			Group = std::make_shared<AsyncOperationGroup>("Loading thumbnails", EAsyncOperationFlags::Background, EAsyncOperationKind::ResourceLoad);
			Manager.Start(Group);
		}

		~GroupedTasks() { Manager.ShutDown(); }

		std::shared_ptr<TestOperation> Add(const std::string& name)
		{
			std::shared_ptr<TestOperation> operation = std::make_shared<TestOperation>(name, EAsyncOperationFlags::Background | EAsyncOperationFlags::SubStep);
			Manager.Start(operation);
			Group->Add(operation);
			return operation;
		}
	};
}

TEST_CASE_FIXTURE(GroupedTasks, "A group's progress is how far its operations have got between them, and it finishes once they all have")
{
	std::shared_ptr<TestOperation> first = Add("Loading a.png");
	std::shared_ptr<TestOperation> second = Add("Loading b.png");
	Manager.Tick();
	CHECK(Group->GetKind() == EAsyncOperationKind::ResourceLoad);
	CHECK(Group->GetStatusText() == "0 of 2 done\nLoading a.png");

	first->SetProgress(0.5f);
	Manager.Tick();
	CHECK(Group->GetProgress() == doctest::Approx(0.25f));

	first->Succeed();
	Manager.Tick();
	Manager.Tick();
	CHECK(Group->GetProgress() == doctest::Approx(0.5f));
	CHECK(Group->GetStatusText() == "1 of 2 done\nLoading b.png");
	CHECK_FALSE(Group->IsFinished());

	second->Succeed();
	Manager.Tick();
	Manager.Tick();
	CHECK(Group->Succeeded());
	CHECK(Group->GetStatusText() == "2 done");
	CHECK_FALSE(Group->HasWarnings());
}

TEST_CASE_FIXTURE(GroupedTasks, "Operations added to a running group count towards it")
{
	std::shared_ptr<TestOperation> first = Add("Loading a.png");
	Manager.Tick();
	first->SetProgress(1.0f);

	std::shared_ptr<TestOperation> second = Add("Loading b.png");
	Manager.Tick();
	CHECK(Group->GetStatusText() == "0 of 2 done\nLoading a.png");
	CHECK(Group->GetProgress() == doctest::Approx(0.5f));

	first->Succeed();
	second->Succeed();
	Manager.Tick();
	Manager.Tick();
	CHECK(Group->Succeeded());
	CHECK(Group->GetStatusText() == "2 done");
}

TEST_CASE_FIXTURE(GroupedTasks, "A failed operation is a warning on the group, and one cancelled on its own just counts as done")
{
	std::shared_ptr<TestOperation> failed = Add("Loading a.png");
	std::shared_ptr<TestOperation> cancelled = Add("Loading b.png");
	Manager.Tick();

	failed->FailWith("the file is missing");
	cancelled->RequestCancel();
	Manager.Tick();
	Manager.Tick();

	CHECK(Group->Succeeded());
	CHECK(Group->GetWarningText() == "Loading a.png failed: the file is missing");
}

TEST_CASE_FIXTURE(GroupedTasks, "Cancelling a group cancels the operations still running")
{
	std::shared_ptr<TestOperation> done = Add("Loading a.png");
	std::shared_ptr<TestOperation> running = Add("Loading b.png");
	Manager.Tick();
	done->Succeed();
	Manager.Tick();

	Group->RequestCancel();
	Manager.Tick();
	CHECK(Group->GetState() == EAsyncOperationState::Cancelled);
	CHECK(running->IsCancellationRequested());
	CHECK_FALSE(done->IsCancellationRequested());
}
