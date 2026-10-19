#include <doctest/doctest.h>

#include <EngineCore/Async/AsyncFrameBudget.h>
#include <EngineCore/Async/AsyncOperation.h>
#include <EngineCore/Async/AsyncOperationManager.h>

#include <TestOperation.h>

#include <memory>
#include <string>
#include <vector>

namespace
{
	std::vector<std::string> GetNames(const std::vector<std::shared_ptr<AsyncOperation>>& operations)
	{
		std::vector<std::string> names;
		for (const std::shared_ptr<AsyncOperation>& operation : operations)
		{
			names.push_back(operation->GetName());
		}
		return names;
	}
}

TEST_CASE("Running operations the filter matches are visible, oldest first, and every running one is reported")
{
	AsyncOperationManager manager;
	manager.Initialize();

	std::shared_ptr<TestOperation> shown = std::make_shared<TestOperation>("Shown", EAsyncOperationFlags::None);
	std::shared_ptr<TestOperation> subStep = std::make_shared<TestOperation>("SubStep", EAsyncOperationFlags::SubStep);
	std::shared_ptr<TestOperation> background = std::make_shared<TestOperation>("Background", EAsyncOperationFlags::Background);
	manager.Start(shown);
	manager.Start(subStep);
	manager.Start(background);
	manager.Tick();

	// Everything shows until the filter says otherwise
	CHECK(GetNames(manager.GetVisibleOperations()) == std::vector<std::string>{ "Shown", "SubStep", "Background" });

	AsyncOperationFilter filter;
	filter.ShowSubSteps = false;
	manager.SetVisibilityFilter(filter);
	CHECK(GetNames(manager.GetVisibleOperations()) == std::vector<std::string>{ "Shown", "Background" });
	CHECK(GetNames(manager.GetRunningOperations()) == std::vector<std::string>{ "Shown", "SubStep", "Background" });

	shown->Succeed();
	manager.Tick();
	CHECK(GetNames(manager.GetVisibleOperations()) == std::vector<std::string>{ "Background" });
	CHECK(GetNames(manager.GetRunningOperations()) == std::vector<std::string>{ "SubStep", "Background" });

	manager.ShutDown();
}

TEST_CASE("Foreground operations are ticked before background ones, whichever started first")
{
	AsyncOperationManager manager;
	manager.Initialize();

	std::vector<std::string> tickOrder;
	manager.Start(std::make_shared<TestOperation>("Background", EAsyncOperationFlags::Background,
		[&tickOrder](AsyncFrameBudget&) { tickOrder.push_back("Background"); }));
	manager.Start(std::make_shared<TestOperation>("Foreground", EAsyncOperationFlags::None,
		[&tickOrder](AsyncFrameBudget&) { tickOrder.push_back("Foreground"); }));

	manager.Tick();
	CHECK(tickOrder == std::vector<std::string>{ "Foreground", "Background" });

	manager.ShutDown();
}

TEST_CASE("A foreground operation that spends its whole budget doesn't stop background ones moving")
{
	AsyncOperationManager manager;
	manager.Initialize();
	manager.SetFrameBudget(1.0, 1024);

	bool foregroundRanOut = false;
	bool backgroundCouldWork = false;
	manager.Start(std::make_shared<TestOperation>("Foreground", EAsyncOperationFlags::None,
		[&foregroundRanOut](AsyncFrameBudget& budget)
		{
			while (budget.TryConsume(ResourceFinalizeCost()))
			{
			}
			foregroundRanOut = budget.IsExhausted();
		}));
	manager.Start(std::make_shared<TestOperation>("Background", EAsyncOperationFlags::Background,
		[&backgroundCouldWork](AsyncFrameBudget& budget)
		{
			backgroundCouldWork = !budget.IsExhausted() && budget.TryConsume(ResourceFinalizeCost());
		}));

	manager.Tick();
	CHECK(foregroundRanOut);
	CHECK(backgroundCouldWork);

	manager.ShutDown();
}

TEST_CASE("Idle ticks give running operations more time, foreground first, with an idle budget")
{
	AsyncOperationManager manager;
	manager.Initialize();

	std::vector<std::string> ticks;
	const auto recordTick = [&ticks](const char* name)
	{
		return [&ticks, name](AsyncFrameBudget& budget) { ticks.push_back(std::string(name) + (budget.IsIdle() ? " idle" : "")); };
	};
	manager.Start(std::make_shared<TestOperation>("Background", EAsyncOperationFlags::Background, recordTick("Background")));
	manager.Start(std::make_shared<TestOperation>("Foreground", EAsyncOperationFlags::None, recordTick("Foreground")));

	manager.Tick();
	manager.TickIdle(5.0);
	CHECK(ticks == std::vector<std::string>{ "Foreground", "Background", "Foreground idle", "Background idle" });

	manager.ShutDown();
}

TEST_CASE("Idle ticks don't finish operations; the next frame's tick does")
{
	AsyncOperationManager manager;
	manager.Initialize();

	int idleTicks = 0;
	std::shared_ptr<TestOperation> operation = std::make_shared<TestOperation>("Done", EAsyncOperationFlags::None,
		[&idleTicks](AsyncFrameBudget& budget) { idleTicks += budget.IsIdle() ? 1 : 0; });
	bool completed = false;
	operation->SetOnCompleted(AsyncOperation::CompletionCallback([&completed](const AsyncOperation&) { completed = true; }));
	manager.Start(operation);
	manager.Tick();

	operation->Succeed();
	manager.TickIdle(5.0);
	CHECK(idleTicks == 0);
	CHECK_FALSE(completed);

	manager.Tick();
	CHECK(completed);

	manager.ShutDown();
}

TEST_CASE("What idle ticks upload comes out of the next frame's allowance, and the frame after starts afresh")
{
	AsyncOperationManager manager;
	manager.Initialize();
	manager.SetFrameBudget(100.0, 1024);

	ResourceFinalizeCost bigUpload;
	bigUpload.UploadBytes = 1000;
	ResourceFinalizeCost smallUpload;
	smallUpload.UploadBytes = 100;

	// The first unit of a budget is always allowed, so the second one shows whether the allowance is left
	bool secondUploadFitted = false;
	manager.Start(std::make_shared<TestOperation>("Uploads", EAsyncOperationFlags::None,
		[&](AsyncFrameBudget& budget)
		{
			if (budget.IsIdle())
			{
				budget.TryConsume(bigUpload);
				return;
			}
			budget.TryConsume(smallUpload);
			secondUploadFitted = budget.TryConsume(smallUpload);
		}));

	manager.Tick();
	CHECK(secondUploadFitted);

	manager.TickIdle(5.0);
	manager.Tick();
	CHECK_FALSE(secondUploadFitted);

	manager.Tick();
	CHECK(secondUploadFitted);

	manager.ShutDown();
}
