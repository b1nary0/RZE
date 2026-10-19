#include <StdAfx.h>
#include <EngineCore/Async/AsyncOperationGroup.h>

#include <algorithm>

AsyncOperationGroup::AsyncOperationGroup(const std::string& name, U32 flags, EAsyncOperationKind kind)
	: AsyncOperation(name, flags, kind)
{
}

void AsyncOperationGroup::Add(const std::shared_ptr<AsyncOperation>& operation)
{
	AssertExpr(!IsFinished());
	m_running.push_back(operation);
}

void AsyncOperationGroup::OnStart()
{
	Update();
}

void AsyncOperationGroup::OnTick(AsyncFrameBudget&)
{
	Update();

	if (m_running.empty())
	{
		SetStatusText(std::to_string(m_doneCount) + " done");
		Complete();
	}
}

void AsyncOperationGroup::OnFinished(EAsyncOperationState finalState)
{
	if (finalState != EAsyncOperationState::Succeeded)
	{
		for (const std::shared_ptr<AsyncOperation>& operation : m_running)
		{
			if (!operation->IsFinished())
			{
				operation->RequestCancel();
			}
		}
	}
	m_running.clear();
}

void AsyncOperationGroup::Update()
{
	const auto firstDone = std::stable_partition(m_running.begin(), m_running.end(),
		[](const std::shared_ptr<AsyncOperation>& operation) { return !operation->IsFinished(); });
	for (auto done = firstDone; done != m_running.end(); ++done)
	{
		const AsyncOperation& operation = **done;
		if (operation.GetState() == EAsyncOperationState::Failed)
		{
			AddWarning(operation.GetName() + " failed: " + operation.GetErrorText());
		}
		++m_doneCount;
	}
	m_running.erase(firstDone, m_running.end());

	const size_t totalCount = m_doneCount + m_running.size();
	if (totalCount == 0)
	{
		return;
	}

	float progress = static_cast<float>(m_doneCount);
	for (const std::shared_ptr<AsyncOperation>& operation : m_running)
	{
		progress += operation->GetProgress();
	}
	SetProgress(progress / static_cast<float>(totalCount));

	std::string status = std::to_string(m_doneCount) + " of " + std::to_string(totalCount) + " done";
	if (!m_running.empty())
	{
		status += "\n" + m_running.front()->GetName();
	}
	SetStatusText(status);
}
