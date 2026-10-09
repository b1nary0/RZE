#pragma once

class AsyncOperationManager;

namespace Editor
{
	// Centered, non-dismissable progress dialog shown while any blocking AsyncOperation is running.
	// Purely an observer of AsyncOperationManager, so any operation flagged EAsyncOperationFlags::Blocking
	// gets this UI for free.
	class AsyncOperationModal
	{
	public:
		AsyncOperationModal() = default;
		~AsyncOperationModal() = default;

	public:
		// Must be called at the root of the ImGui ID stack (i.e. outside of any Begin/End pair).
		void Display(const AsyncOperationManager& operationManager);
	};
}
