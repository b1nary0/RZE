#include <doctest/doctest.h>

#include <RZE.h>

#include <TestComponents.h>

#include <Services/GameObjectClipboard.h>
#include <Services/GameObjectSnapshot.h>

#include <Game/GameScene.h>
#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/TransformComponent.h>

#include <string>
#include <vector>

using namespace Editor;

namespace
{
	std::string KeepName(const std::string& name)
	{
		return name;
	}

	struct SnapshotFixture
	{
		GameScene Scene;

		SnapshotFixture()
		{
			RegisterTestComponents();
		}

		~SnapshotFixture()
		{
			Scene.Unload();
		}

		GameObjectPtr AddWithTestComponent(const char* name)
		{
			GameObjectPtr gameObject = Scene.AddGameObject(name);
			gameObject->AddComponent<TestComponent>();
			return gameObject;
		}

		GameObjectPtr Paste(const GameObjectPtr& original, const GameObjectPtr& parent = nullptr)
		{
			return GameObjectSnapshot::Take(**original).Create(Scene, parent, &KeepName);
		}
	};
}

TEST_CASE_FIXTURE(SnapshotFixture, "A copy is a new object with the original's name, components and transform")
{
	GameObjectPtr original = AddWithTestComponent("Tank");
	original->GetTransformComponent()->SetPosition(Vector3D(1.0f, 2.0f, 3.0f));
	original->GetComponent<TestComponent>()->SetSpeed(7.0f);

	GameObjectPtr copy = Paste(original);

	REQUIRE(copy != nullptr);
	CHECK(copy != original);
	CHECK(copy->GetGUID() != original->GetGUID());
	CHECK(copy->GetName() == "Tank");
	CHECK(copy->GetTransformComponent()->GetPosition().X() == 1.0f);
	CHECK(copy->GetTransformComponent()->GetPosition().Z() == 3.0f);
	REQUIRE(copy->GetComponent<TestComponent>() != nullptr);
	CHECK(copy->GetComponent<TestComponent>()->GetSpeed() == 7.0f);
	CHECK(copy->IsInScene());
}

TEST_CASE_FIXTURE(SnapshotFixture, "Copying skips EditorOnly and Exclusive properties, and keeps ReadOnly ones")
{
	GameObjectPtr original = AddWithTestComponent("Leader");
	GameObjectComponentPtr<TestComponent> component = original->GetComponent<TestComponent>();
	component->SetDrawDebug(true);
	component->SetLeader(true);
	component->SetSerial(42);

	GameObjectPtr copy = Paste(original);
	GameObjectComponentPtr<TestComponent> copied = copy->GetComponent<TestComponent>();

	CHECK_FALSE(copied->IsDrawingDebug());
	CHECK_FALSE(copied->IsLeader());
	CHECK(copied->GetSerial() == 42);
}

TEST_CASE_FIXTURE(SnapshotFixture, "Values are taken when the snapshot is, not when it's used")
{
	GameObjectPtr original = AddWithTestComponent("Tank");
	original->GetComponent<TestComponent>()->SetSpeed(3.0f);

	const GameObjectSnapshot snapshot = GameObjectSnapshot::Take(**original);
	original->GetComponent<TestComponent>()->SetSpeed(9.0f);

	GameObjectPtr copy = snapshot.Create(Scene, nullptr, &KeepName);
	CHECK(copy->GetComponent<TestComponent>()->GetSpeed() == 3.0f);
}

TEST_CASE_FIXTURE(SnapshotFixture, "Children are copied in order under the copy")
{
	GameObjectPtr parent = Scene.AddGameObject("Parent");
	GameObjectPtr first = Scene.AddGameObject("First");
	GameObjectPtr second = Scene.AddGameObject("Second");
	first->AttachTo(parent);
	second->AttachTo(parent);
	second->GetTransformComponent()->SetPosition(Vector3D(5.0f, 0.0f, 0.0f));

	GameObjectPtr copy = Paste(parent);

	REQUIRE(copy->NumChildren() == 2);
	CHECK(copy->GetChildAtIndex(0)->GetName() == "First");
	CHECK(copy->GetChildAtIndex(1)->GetName() == "Second");
	CHECK(copy->GetChildAtIndex(1) != second);
	// Local to the parent, so the same as the original's
	CHECK(copy->GetChildAtIndex(1)->GetTransformComponent()->GetPosition().X() == 5.0f);
	CHECK(parent->NumChildren() == 2);
}

TEST_CASE_FIXTURE(SnapshotFixture, "The snapshot remembers the original's parent, and Create attaches to the parent it's given")
{
	GameObjectPtr parent = Scene.AddGameObject("Parent");
	GameObjectPtr child = Scene.AddGameObject("Child");
	child->AttachTo(parent);

	const GameObjectSnapshot snapshot = GameObjectSnapshot::Take(**child);
	CHECK(snapshot.GetParentGUID() == parent->GetGUID());

	GameObjectPtr copy = snapshot.Create(Scene, parent, &KeepName);
	CHECK(copy->GetParent() == parent);
	CHECK(parent->NumChildren() == 2);
}

TEST_CASE_FIXTURE(SnapshotFixture, "An object that isn't saved copies as one that isn't either")
{
	GameObjectPtr original = Scene.AddGameObject("Helper");
	original->SetIncludeInSave(false);
	GameObjectPtr child = Scene.AddGameObject("Helper Child");
	child->SetIncludeInSave(false);
	child->AttachTo(original);

	const std::vector<GameObjectSnapshot> snapshots = GameObjectClipboard::TakeSnapshots({ original });
	REQUIRE(snapshots.size() == 1);

	GameObjectPtr copy = snapshots[0].Create(Scene, nullptr, &KeepName);
	CHECK_FALSE(copy->IncludeInSave());
	REQUIRE(copy->NumChildren() == 1);
	CHECK_FALSE(copy->GetChildAtIndex(0)->IncludeInSave());
}

TEST_CASE_FIXTURE(SnapshotFixture, "Each object is named by the given function")
{
	GameObjectPtr parent = Scene.AddGameObject("Parent");
	Scene.AddGameObject("Child")->AttachTo(parent);

	GameObjectPtr copy = GameObjectSnapshot::Take(**parent).Create(Scene, nullptr,
		[](const std::string& name) { return name + " copy"; });

	CHECK(copy->GetName() == "Parent copy");
	CHECK(copy->GetChildAtIndex(0)->GetName() == "Child copy");
}

TEST_CASE_FIXTURE(SnapshotFixture, "TakeSnapshots copies an object under another selected one only once")
{
	GameObjectPtr parent = Scene.AddGameObject("Parent");
	GameObjectPtr child = Scene.AddGameObject("Child");
	child->AttachTo(parent);
	GameObjectPtr other = Scene.AddGameObject("Other");

	const std::vector<GameObjectSnapshot> snapshots = GameObjectClipboard::TakeSnapshots({ child, parent, other });

	CHECK(snapshots.size() == 2);
}
