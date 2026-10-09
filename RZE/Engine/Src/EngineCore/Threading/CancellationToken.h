#pragma once

#include <EngineCore/Threading/Threading.h>

namespace Threading
{
	// Cooperative cancellation flag. Copies share the same underlying state, so a token can be
	// captured by value into worker jobs and observed from any thread.
	class CancellationToken
	{
	public:
		CancellationToken()
			: m_isCancelled(std::make_shared<std::atomic<bool>>(false))
		{
		}

	public:
		void RequestCancel() { m_isCancelled->store(true, std::memory_order_release); }
		bool IsCancellationRequested() const { return m_isCancelled->load(std::memory_order_acquire); }

	private:
		std::shared_ptr<std::atomic<bool>> m_isCancelled;
	};
}
