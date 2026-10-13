#include <doctest/doctest.h>

#include <EngineCore/Async/AsyncFrameBudget.h>
#include <EngineCore/Async/AsyncOperation.h>
#include <EngineCore/Async/AsyncOperationManager.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace
{
	// Runs onTick each frame until completed from outside
	class TestOperation final : public AsyncOperation
	{
	public:
		TestOperation(const std::string& name, U32 flags, std::function<void(AsyncFrameBudget&)> onTick = nullptr)
			: AsyncOperation(name, flags)
			, m_onTick(std::move(onTick)) {}

		void Finish() { Complete(); }

	protected:
		void OnStart() override {}
		void OnTick(AsyncFrameBudget& budget) override
		{
			if (m_onTick)
			{
				m_onTick(budget);
			}
		}

	private:
		std::function<void(AsyncFrameBudget&)> m_onTick;
	};

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

	shown->Finish();
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
