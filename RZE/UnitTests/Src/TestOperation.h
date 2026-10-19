#pragma once

#include <EngineCore/Async/AsyncOperation.h>

#include <functional>
#include <string>

// An operation that runs onTick each frame and finishes when a test says so, for tests of AsyncOperationManager and of
// what follows its operations
class TestOperation final : public AsyncOperation
{
public:
	TestOperation(const std::string& name, U32 flags, std::function<void(AsyncFrameBudget&)> onTick = nullptr,
		EAsyncOperationKind kind = EAsyncOperationKind::General)
		: AsyncOperation(name, flags, kind)
		, m_onTick(std::move(onTick)) {}

	// Each takes effect at the manager's next Tick
	void Succeed() { Complete(); }
	void FailWith(const std::string& reason) { Fail(reason); }

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
