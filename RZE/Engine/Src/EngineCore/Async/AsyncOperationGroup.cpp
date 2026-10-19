#include <StdAfx.h>
#include <EngineCore/Async/AsyncOperationGroup.h>

#include <algorithm>

AsyncOperationGroup::AsyncOperationGroup(const std::string& name, U32 flags, EAsyncOperationKind kind)
	: AsyncOperation(name, flags, kind)
{
}

void AsyncOperationGroup::Add(const std::shared_ptr<AsyncOperation>& operation, const std::string& label)
{
	AssertExpr(!IsFinished());
	m_running.push_back({ operation, label.empty() ? operation->GetName() : label });
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
		SetStatusText(FormatSummary());
		Complete();
	}
}

void AsyncOperationGroup::OnFinished(EAsyncOperationState finalState)
{
	if (finalState != EAsyncOperationState::Succeeded)
	{
		for (const Item& item : m_running)
		{
			if (!item.Operation->IsFinished())
			{
				item.Operation->RequestCancel();
			}
		}
	}
	m_running.clear();
}

void AsyncOperationGroup::Update()
{
	const auto firstDone = std::stable_partition(m_running.begin(), m_running.end(),
		[](const Item& item) { return !item.Operation->IsFinished(); });
	for (auto done = firstDone; done != m_running.end(); ++done)
	{
		const AsyncOperation& operation = *done->Operation;
		switch (operation.GetState())
		{
		case EAsyncOperationState::Failed:
			AddWarning(done->Label + " failed: " + operation.GetErrorText());
			++m_failedCount;
			break;
		case EAsyncOperationState::Cancelled:
			LogStep(done->Label + ": cancelled");
			++m_cancelledCount;
			break;
		default:
			LogStep(done->Label);
			break;
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
	for (const Item& item : m_running)
	{
		progress += item.Operation->GetProgress();
	}
	SetProgress(progress / static_cast<float>(totalCount));

	std::string status = std::to_string(m_doneCount) + " of " + std::to_string(totalCount) + " done";
	if (!m_running.empty())
	{
		status += "\n" + m_running.front().Operation->GetName();
	}
	SetStatusText(status);
}

std::string AsyncOperationGroup::FormatSummary() const
{
	std::string summary = std::to_string(m_doneCount - m_cancelledCount - m_failedCount) + " done";
	if (m_cancelledCount > 0)
	{
		summary += ", " + std::to_string(m_cancelledCount) + " cancelled";
	}
	if (m_failedCount > 0)
	{
		summary += ", " + std::to_string(m_failedCount) + " failed";
	}
	return summary;
}
