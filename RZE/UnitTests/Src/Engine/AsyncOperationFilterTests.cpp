#include <doctest/doctest.h>

#include <EngineCore/Async/AsyncOperation.h>
#include <EngineCore/Async/AsyncOperationFilter.h>

#include <TestOperation.h>

TEST_CASE("The filter shows every operation by default")
{
	const AsyncOperationFilter filter;
	for (const EAsyncOperationKind kind : { EAsyncOperationKind::General, EAsyncOperationKind::ResourceLoad, EAsyncOperationKind::SceneLoad,
		EAsyncOperationKind::Rendering, EAsyncOperationKind::Saving })
	{
		CHECK(filter.Matches(TestOperation("Task", EAsyncOperationFlags::None, nullptr, kind)));
		CHECK(filter.Matches(TestOperation("Task", EAsyncOperationFlags::SubStep | EAsyncOperationFlags::Background, nullptr, kind)));
		CHECK(filter.Matches(TestOperation("Task", EAsyncOperationFlags::Blocking, nullptr, kind)));
	}
}

TEST_CASE("The filter leaves out sub-steps and blocking operations when asked to, whatever their kind")
{
	const TestOperation plain("Plain", EAsyncOperationFlags::Background, nullptr, EAsyncOperationKind::ResourceLoad);
	const TestOperation subStep("SubStep", EAsyncOperationFlags::Background | EAsyncOperationFlags::SubStep, nullptr, EAsyncOperationKind::ResourceLoad);
	const TestOperation blocking("Blocking", EAsyncOperationFlags::Blocking, nullptr, EAsyncOperationKind::SceneLoad);

	AsyncOperationFilter filter;
	filter.ShowSubSteps = false;
	CHECK(filter.Matches(plain));
	CHECK_FALSE(filter.Matches(subStep));
	CHECK(filter.Matches(blocking));

	filter = AsyncOperationFilter();
	filter.ShowBlocking = false;
	CHECK(filter.Matches(plain));
	CHECK(filter.Matches(subStep));
	CHECK_FALSE(filter.Matches(blocking));
}

TEST_CASE("The filter's switch for each kind shows or leaves out that kind only")
{
	const TestOperation resourceLoad("ResourceLoad", EAsyncOperationFlags::None, nullptr, EAsyncOperationKind::ResourceLoad);
	const TestOperation sceneLoad("SceneLoad", EAsyncOperationFlags::None, nullptr, EAsyncOperationKind::SceneLoad);
	const TestOperation rendering("Rendering", EAsyncOperationFlags::None, nullptr, EAsyncOperationKind::Rendering);
	const TestOperation saving("Saving", EAsyncOperationFlags::None, nullptr, EAsyncOperationKind::Saving);
	const TestOperation general("General", EAsyncOperationFlags::None, nullptr, EAsyncOperationKind::General);
	const TestOperation* const operations[] = { &resourceLoad, &sceneLoad, &rendering, &saving, &general };

	for (const TestOperation* const hidden : operations)
	{
		CAPTURE(hidden->GetName());
		AsyncOperationFilter filter;
		filter.SetShowsKind(hidden->GetKind(), false);
		CHECK_FALSE(filter.ShowsKind(hidden->GetKind()));
		for (const TestOperation* const operation : operations)
		{
			CAPTURE(operation->GetName());
			CHECK(filter.Matches(*operation) == (operation != hidden));
		}

		filter.SetShowsKind(hidden->GetKind(), true);
		CHECK(filter.ShownKinds == AsyncOperationFilter::k_allKinds);
	}
}
