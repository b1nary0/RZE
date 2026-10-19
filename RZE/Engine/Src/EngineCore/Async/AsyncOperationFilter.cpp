#include <StdAfx.h>
#include <EngineCore/Async/AsyncOperationFilter.h>

bool AsyncOperationFilter::ShowsKind(EAsyncOperationKind kind) const
{
	return (ShownKinds & static_cast<U32>(kind)) != 0;
}

void AsyncOperationFilter::SetShowsKind(EAsyncOperationKind kind, bool isShown)
{
	ShownKinds = isShown ? ShownKinds | static_cast<U32>(kind) : ShownKinds & ~static_cast<U32>(kind);
}

bool AsyncOperationFilter::Matches(const AsyncOperation& operation) const
{
	if ((operation.IsSubStep() && !ShowSubSteps) || (operation.IsBlocking() && !ShowBlocking))
	{
		return false;
	}
	return ShowsKind(operation.GetKind());
}
