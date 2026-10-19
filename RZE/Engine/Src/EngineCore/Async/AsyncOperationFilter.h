#pragma once

#include <EngineCore/Async/AsyncOperation.h>

#include <Utils/PrimitiveDefs.h>

// Which operations a progress display shows (AsyncOperationManager::GetVisibleOperations). Operations only say what
// they are (EAsyncOperationKind, EAsyncOperationFlags::SubStep); this picks among them. Engine.ini's [BackgroundTasks]
// gives the defaults (EngineConfig), and AsyncOperationManager::SetVisibilityFilter changes them while running.
struct AsyncOperationFilter
{
	static constexpr U32 k_allKinds = static_cast<U32>(EAsyncOperationKind::General) | static_cast<U32>(EAsyncOperationKind::ResourceLoad)
		| static_cast<U32>(EAsyncOperationKind::SceneLoad) | static_cast<U32>(EAsyncOperationKind::Rendering) | static_cast<U32>(EAsyncOperationKind::Saving);

	// Those that are part of another operation (EAsyncOperationFlags::SubStep)
	bool ShowSubSteps = true;
	// Those that block interaction, which have a progress dialog of their own (EAsyncOperationFlags::Blocking)
	bool ShowBlocking = true;
	// The kinds shown, a bit each (EAsyncOperationKind)
	U32 ShownKinds = k_allKinds;

	bool ShowsKind(EAsyncOperationKind kind) const;
	void SetShowsKind(EAsyncOperationKind kind, bool isShown);

	bool Matches(const AsyncOperation& operation) const;
};
