#include <doctest/doctest.h>

#include <RZE.h>

#include <Game/GameScene.h>
#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/TransformComponent.h>

#include <TestComponents.h>

#include <RapidJSON/document.h>

#include <algorithm>

namespace
{
	// The TransformComponent among the object's components, found the slow way
	const GameObjectComponentBase* FindTransformInList(const GameObject* object)
	{
		const GameObject::ComponentList& components = object->GetComponents();
		const auto it = std::find_if(components.begin(), components.end(),
			[](const GameObjectComponentBase* component) { return component->GetTypeID() == TransformComponent::StaticTypeID(); });
		return it != components.end() ? *it : nullptr;
	}
}

TEST_CASE("An object's TransformComponent is kept, and both ways of asking give the one it has")
{
	GameScene scene;
	GameObjectPtr object = scene.AddGameObject("Object");

	const GameObjectComponentBase* const listed = FindTransformInList(*object);
	REQUIRE(listed != nullptr);
	CHECK(*object->GetTransformComponent() == listed);
	CHECK(*object->GetComponent<TransformComponent>() == listed);

	scene.Unload();
}

TEST_CASE("A loaded object keeps its TransformComponent wherever it comes in the file, and lets it go on Uninitialize")
{
	RegisterTestComponents();
	GameScene scene;

	// Another component first, as a hand-edited file might have it
	rapidjson::Document entry;
	entry.Parse(R"({"components": {
		"TestComponent": {},
		"TransformComponent": {"Position": [1.0, 2.0, 3.0], "Scale": [1.0, 1.0, 1.0], "Rotation": [0.0, 0.0, 0.0]}
	}})");
	REQUIRE_FALSE(entry.HasParseError());

	GameObject object(scene, "Loaded", ObjectGUID(0x1234));
	object.LoadComponents(entry);

	REQUIRE(object.GetComponents().size() == 2);
	const GameObjectComponentBase* const listed = FindTransformInList(&object);
	REQUIRE(listed != nullptr);
	CHECK(*object.GetComponent<TransformComponent>() == listed);
	CHECK(object.GetTransformComponent()->GetPosition().Y() == doctest::Approx(2.0f));

	object.Uninitialize();
	CHECK(object.GetComponent<TransformComponent>() == nullptr);
}

TEST_CASE("An object is a root exactly when its TransformComponent is")
{
	GameScene scene;
	GameObjectPtr parent = scene.AddGameObject("Parent");
	GameObjectPtr child = scene.AddGameObject("Child");
	CHECK(child->IsRoot());
	CHECK(child->GetTransformComponent()->IsRoot());

	child->AttachTo(parent);
	CHECK_FALSE(child->IsRoot());
	CHECK_FALSE(child->GetTransformComponent()->IsRoot());
	CHECK(parent->IsRoot());

	child->DetachFromParent();
	CHECK(child->IsRoot());
	CHECK(child->GetTransformComponent()->IsRoot());

	scene.Unload();
}
