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

TEST_CASE("Only running operations that aren't hidden are visible, oldest first")
{
	AsyncOperationManager manager;
	manager.Initialize();

	std::shared_ptr<TestOperation> shown = std::make_shared<TestOperation>("Shown", EAsyncOperationFlags::None);
	std::shared_ptr<TestOperation> hidden = std::make_shared<TestOperation>("Hidden", EAsyncOperationFlags::Hidden);
	std::shared_ptr<TestOperation> background = std::make_shared<TestOperation>("Background", EAsyncOperationFlags::Background);
	manager.Start(shown);
	manager.Start(hidden);
	manager.Start(background);
	manager.Tick();

	CHECK(GetNames(manager.GetVisibleOperations()) == std::vector<std::string>{ "Shown", "Background" });

	shown->Succeed();
	manager.Tick();
	CHECK(GetNames(manager.GetVisibleOperations()) == std::vector<std::string>{ "Background" });

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
