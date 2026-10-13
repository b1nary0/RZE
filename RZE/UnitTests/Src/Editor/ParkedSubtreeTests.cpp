#include <doctest/doctest.h>

#include <RZE.h>

#include <Commands/ParkedSubtree.h>
#include <UI/Framework/EditorEvents.h>
#include <UI/Framework/EditorSelection.h>

#include <Game/GameScene.h>
#include <Game/World/GameObject/GameObject.h>

#include <string>
#include <vector>

using namespace Editor;

namespace
{
	struct ParkFixture
	{
		GameScene Scene;
		EditorEvents Events;
		EditorSelection Selection{ Events };

		// Roots A, Parent, C; Parent has children First, Middle, Last
		GameObjectPtr A = Scene.AddGameObject("A");
		GameObjectPtr Parent = Scene.AddGameObject("Parent");
		GameObjectPtr C = Scene.AddGameObject("C");
		GameObjectPtr First = AddChild("First");
		GameObjectPtr Middle = AddChild("Middle");
		GameObjectPtr Last = AddChild("Last");

		~ParkFixture()
		{
			Scene.Unload();
		}

		GameObjectPtr AddChild(const char* name)
		{
			GameObjectPtr child = Scene.AddGameObject(name);
			child->AttachTo(Parent);
			return child;
		}

		// Names of the objects in the scene, in the scene's order
		std::vector<std::string> SceneNames()
		{
			std::vector<std::string> names;
			Scene.ForEachGameObject(Functor<void, GameObjectPtr>([&names](GameObjectPtr gameObject)
				{
					names.push_back(gameObject->GetName());
				}));
			return names;
		}
	};
}

TEST_CASE_FIXTURE(ParkFixture, "Parking takes the subtree out of the scene but keeps the objects alive")
{
	ParkedSubtree subtree(Parent);
	subtree.Park(Scene, Selection);

	CHECK(subtree.IsParked());
	CHECK(Parent != nullptr);
	CHECK(Middle != nullptr);
	CHECK_FALSE(Parent->IsInScene());
	CHECK_FALSE(Middle->IsInScene());
	CHECK(Scene.FindGameObjectByGUID(Middle->GetGUID()) == nullptr);
	CHECK(SceneNames() == std::vector<std::string>{ "A", "C" });
}

TEST_CASE_FIXTURE(ParkFixture, "Restoring puts back the same objects in the same order")
{
	ParkedSubtree subtree(Parent);
	subtree.Park(Scene, Selection);
	subtree.Restore(Scene);

	CHECK_FALSE(subtree.IsParked());
	CHECK(Scene.FindGameObjectByGUID(Middle->GetGUID()) == Middle);
	CHECK(SceneNames() == std::vector<std::string>{ "A", "Parent", "C", "First", "Middle", "Last" });
	REQUIRE(Parent->NumChildren() == 3);
	CHECK(Parent->GetChildAtIndex(1) == Middle);
}

TEST_CASE_FIXTURE(ParkFixture, "A parked child goes back at its index under its parent")
{
	ParkedSubtree subtree(Middle);
	subtree.Park(Scene, Selection);

	REQUIRE(Parent->NumChildren() == 2);
	CHECK(Parent->GetChildAtIndex(1) == Last);

	subtree.Restore(Scene);

	REQUIRE(Parent->NumChildren() == 3);
	CHECK(Parent->GetChildAtIndex(0) == First);
	CHECK(Parent->GetChildAtIndex(1) == Middle);
	CHECK(Parent->GetChildAtIndex(2) == Last);
	CHECK(Middle->GetParent() == Parent);
}

TEST_CASE_FIXTURE(ParkFixture, "Parking takes the subtree out of the selection and keeps the rest")
{
	Selection.SetSelection({ A, Middle, C });

	ParkedSubtree subtree(Parent);
	subtree.Park(Scene, Selection);

	CHECK(Selection.GetAll() == std::vector<GameObjectPtr>{ A, C });
}

TEST_CASE_FIXTURE(ParkFixture, "DestroyIfParked destroys parked objects and leaves objects in the scene alone")
{
	ParkedSubtree inScene(A);
	inScene.DestroyIfParked(Scene);
	Scene.Update();
	CHECK(A != nullptr);

	ParkedSubtree parked(Parent);
	parked.Park(Scene, Selection);
	parked.DestroyIfParked(Scene);
	// Removal is deferred to the scene's next update
	Scene.Update();

	CHECK(Parent == nullptr);
	CHECK(Middle == nullptr);
	CHECK(SceneNames() == std::vector<std::string>{ "A", "C" });
}
