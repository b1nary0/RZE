#include <doctest/doctest.h>

#include <RZE.h>

#include <UI/Framework/EditorEvents.h>
#include <UI/Framework/EditorSelection.h>

#include <Game/GameScene.h>
#include <Game/World/GameObject/GameObject.h>

#include <vector>

using namespace Editor;

namespace
{
	struct SelectionFixture
	{
		GameScene Scene;
		EditorEvents Events;
		EditorSelection Selection{ Events };
		int ChangeCount = 0;
		EventSubscription Changed = Events.Subscribe<SelectionChangedEvent>([this](const SelectionChangedEvent&) { ++ChangeCount; });

		GameObjectPtr A = Scene.AddGameObject("A");
		GameObjectPtr B = Scene.AddGameObject("B");
		GameObjectPtr C = Scene.AddGameObject("C");

		~SelectionFixture()
		{
			Scene.Unload();
		}
	};
}

TEST_CASE_FIXTURE(SelectionFixture, "A new selection is empty")
{
	CHECK(Selection.IsEmpty());
	CHECK(Selection.GetPrimary() == nullptr);
}

TEST_CASE_FIXTURE(SelectionFixture, "Select replaces the selection")
{
	Selection.SetSelection({ A, B });
	Selection.Select(C);

	CHECK(Selection.GetAll() == std::vector<GameObjectPtr>{ C });
	CHECK(Selection.GetPrimary() == C);
	CHECK(ChangeCount == 2);
}

TEST_CASE_FIXTURE(SelectionFixture, "SetSelection drops duplicates, keeps order, and makes the last one primary")
{
	Selection.SetSelection({ A, B, A, C });

	CHECK(Selection.GetAll() == std::vector<GameObjectPtr>{ A, B, C });
	CHECK(Selection.GetPrimary() == C);
	CHECK(Selection.IsSelected(B));
	CHECK(ChangeCount == 1);
}

TEST_CASE_FIXTURE(SelectionFixture, "Setting the same selection again doesn't publish")
{
	Selection.SetSelection({ A, B });
	Selection.SetSelection({ A, B });

	CHECK(ChangeCount == 1);
}

TEST_CASE_FIXTURE(SelectionFixture, "Toggle adds an object as primary, then removes it")
{
	Selection.Select(A);
	Selection.Toggle(B);
	CHECK(Selection.GetAll() == std::vector<GameObjectPtr>{ A, B });
	CHECK(Selection.GetPrimary() == B);

	Selection.Toggle(B);
	CHECK(Selection.GetAll() == std::vector<GameObjectPtr>{ A });
	CHECK(ChangeCount == 3);
}

TEST_CASE_FIXTURE(SelectionFixture, "AddToSelection adds only what isn't selected, and publishes only on a change")
{
	Selection.Select(A);
	Selection.AddToSelection({ A, B });
	CHECK(Selection.GetAll() == std::vector<GameObjectPtr>{ A, B });
	CHECK(ChangeCount == 2);

	Selection.AddToSelection({ A, B });
	CHECK(ChangeCount == 2);
}

TEST_CASE_FIXTURE(SelectionFixture, "Clear publishes only when something was selected")
{
	Selection.Clear();
	CHECK(ChangeCount == 0);

	Selection.Select(A);
	Selection.Clear();
	CHECK(Selection.IsEmpty());
	CHECK(ChangeCount == 2);
}
