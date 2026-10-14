#include <doctest/doctest.h>

#include <RZE.h>

#include <Game/GameScene.h>
#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/TransformComponent.h>

#include <Utils/Math/BoundingBox.h>

#include <TestComponents.h>

#include <string>

namespace
{
	// A metre across, around its object's origin
	const BoundingBox k_unitBox(Vector3D(-0.5f, -0.5f, -0.5f), Vector3D(0.5f, 0.5f, 0.5f));

	void CheckBox(const BoundingBox& box, const Vector3D& expectedMin, const Vector3D& expectedMax)
	{
		REQUIRE_FALSE(box.IsEmpty());
		for (int axis = 0; axis < 3; ++axis)
		{
			CHECK(box.GetMin()[axis] == doctest::Approx(expectedMin[axis]));
			CHECK(box.GetMax()[axis] == doctest::Approx(expectedMax[axis]));
		}
	}

	// Objects in a scene, each with a box of its own if it's given one
	struct BoundedObjects
	{
		GameScene Scene;

		BoundedObjects() { RegisterTestComponents(); }
		~BoundedObjects() { Scene.Unload(); }

		// At position, in parent if it has one
		GameObjectPtr Add(const std::string& name, const BoundingBox& box, const Vector3D& position, GameObjectPtr parent = nullptr)
		{
			GameObjectPtr object = Scene.AddGameObject(name);
			if (parent != nullptr)
			{
				object->AttachTo(parent);
			}
			object->GetTransformComponent()->SetPosition(position);
			if (!box.IsEmpty())
			{
				object->AddComponent<BoundsTestComponent>()->SetBounds(box);
			}
			return object;
		}

		// Brings the objects' world transforms up to date, as a frame does
		void Frame() { Scene.Update(); }

		// How many times the object has worked its own extents out
		static int TimesWorkedOut(const GameObjectPtr& object) { return object->GetComponent<BoundsTestComponent>()->GetTimesAsked(); }
	};
}

TEST_CASE_FIXTURE(BoundedObjects, "An object's bounds are its components' boxes, placed by its transform")
{
	GameObjectPtr object = Add("Object", k_unitBox, Vector3D(10.0f, 0.0f, 0.0f));
	Frame();
	CheckBox(object->GetHierarchyBounds(), Vector3D(9.5f, -0.5f, -0.5f), Vector3D(10.5f, 0.5f, 0.5f));

	// Stretched along X, then turned a quarter about Y, so the stretch lies along Z
	object->GetTransformComponent()->SetScale(Vector3D(2.0f, 1.0f, 1.0f));
	object->GetTransformComponent()->SetRotation(Vector3D(0.0f, 90.0f, 0.0f));
	Frame();
	const BoundingBox bounds = object->GetHierarchyBounds();
	CHECK(bounds.GetSize().X() == doctest::Approx(1.0f));
	CHECK(bounds.GetSize().Y() == doctest::Approx(1.0f));
	CHECK(bounds.GetSize().Z() == doctest::Approx(2.0f));
	CHECK(bounds.GetCentre().X() == doctest::Approx(10.0f));
}

TEST_CASE_FIXTURE(BoundedObjects, "Moving, turning or scaling a root reuses its extents, and places them right")
{
	GameObjectPtr object = Add("Object", k_unitBox, Vector3D());
	Frame();
	object->GetHierarchyBounds();
	REQUIRE(TimesWorkedOut(object) == 1);

	object->GetTransformComponent()->SetPosition(Vector3D(3.0f, 4.0f, 5.0f));
	Frame();
	CheckBox(object->GetHierarchyBounds(), Vector3D(2.5f, 3.5f, 4.5f), Vector3D(3.5f, 4.5f, 5.5f));

	object->GetTransformComponent()->SetRotation(Vector3D(0.0f, 0.0f, 45.0f));
	object->GetTransformComponent()->SetScale(Vector3D(2.0f));
	Frame();
	const BoundingBox bounds = object->GetHierarchyBounds();
	// A 2 m square turned 45 degrees about Z reaches out its half-diagonal along X and Y
	CHECK(bounds.GetSize().X() == doctest::Approx(2.0f * std::sqrt(2.0f)));
	CHECK(bounds.GetSize().Y() == doctest::Approx(2.0f * std::sqrt(2.0f)));
	CHECK(bounds.GetSize().Z() == doctest::Approx(2.0f));
	CHECK(bounds.GetCentre().Z() == doctest::Approx(5.0f));

	CHECK(TimesWorkedOut(object) == 1);
}

TEST_CASE_FIXTURE(BoundedObjects, "Moving a parent reuses its extents and its children's")
{
	GameObjectPtr parent = Add("Parent", k_unitBox, Vector3D());
	GameObjectPtr child = Add("Child", k_unitBox, Vector3D(5.0f, 0.0f, 0.0f), parent);
	Frame();
	CheckBox(parent->GetHierarchyBounds(), Vector3D(-0.5f, -0.5f, -0.5f), Vector3D(5.5f, 0.5f, 0.5f));
	const int parentTimes = TimesWorkedOut(parent);
	const int childTimes = TimesWorkedOut(child);

	parent->GetTransformComponent()->SetPosition(Vector3D(100.0f, 0.0f, 0.0f));
	Frame();
	CheckBox(parent->GetHierarchyBounds(), Vector3D(99.5f, -0.5f, -0.5f), Vector3D(105.5f, 0.5f, 0.5f));
	// The child goes with it
	CheckBox(child->GetHierarchyBounds(), Vector3D(104.5f, -0.5f, -0.5f), Vector3D(105.5f, 0.5f, 0.5f));

	CHECK(TimesWorkedOut(parent) == parentTimes);
	CHECK(TimesWorkedOut(child) == childTimes);
}

TEST_CASE_FIXTURE(BoundedObjects, "Moving a child within its parent works the parent's extents out again, and reuses the child's")
{
	GameObjectPtr parent = Add("Parent", k_unitBox, Vector3D());
	GameObjectPtr child = Add("Child", k_unitBox, Vector3D(5.0f, 0.0f, 0.0f), parent);
	GameObjectPtr sibling = Add("Sibling", k_unitBox, Vector3D(0.0f, 3.0f, 0.0f), parent);
	Frame();
	CheckBox(parent->GetHierarchyBounds(), Vector3D(-0.5f, -0.5f, -0.5f), Vector3D(5.5f, 3.5f, 0.5f));
	const int parentTimes = TimesWorkedOut(parent);
	const int childTimes = TimesWorkedOut(child);
	const int siblingTimes = TimesWorkedOut(sibling);

	// To the other side: the box has to shrink where the child was, not only grow where it went
	child->GetTransformComponent()->SetPosition(Vector3D(-5.0f, 0.0f, 0.0f));
	Frame();
	CheckBox(parent->GetHierarchyBounds(), Vector3D(-5.5f, -0.5f, -0.5f), Vector3D(0.5f, 3.5f, 0.5f));

	CHECK(TimesWorkedOut(parent) == parentTimes + 1);
	CHECK(TimesWorkedOut(child) == childTimes);
	CHECK(TimesWorkedOut(sibling) == siblingTimes);
}

TEST_CASE_FIXTURE(BoundedObjects, "A change deep down reaches the root, even once something in between was asked on its own")
{
	GameObjectPtr root = Add("Root", BoundingBox(), Vector3D());
	GameObjectPtr middle = Add("Middle", BoundingBox(), Vector3D(0.0f, 0.0f, 10.0f), root);
	GameObjectPtr leaf = Add("Leaf", k_unitBox, Vector3D(0.0f, 0.0f, 10.0f), middle);
	Frame();
	CheckBox(root->GetHierarchyBounds(), Vector3D(-0.5f, -0.5f, 19.5f), Vector3D(0.5f, 0.5f, 20.5f));

	leaf->GetComponent<BoundsTestComponent>()->SetBounds(BoundingBox(Vector3D(-1.0f), Vector3D(1.0f)));
	// The middle is worked out again on its own, while the root still waits to be
	CheckBox(middle->GetHierarchyBounds(), Vector3D(-1.0f, -1.0f, 19.0f), Vector3D(1.0f, 1.0f, 21.0f));

	// Marking stops at the root, which is still waiting: the middle has to be marked again on the way
	leaf->GetComponent<BoundsTestComponent>()->SetBounds(BoundingBox(Vector3D(-2.0f), Vector3D(2.0f)));
	CheckBox(root->GetHierarchyBounds(), Vector3D(-2.0f, -2.0f, 18.0f), Vector3D(2.0f, 2.0f, 22.0f));
	CheckBox(middle->GetHierarchyBounds(), Vector3D(-2.0f, -2.0f, 18.0f), Vector3D(2.0f, 2.0f, 22.0f));

	// And moving the leaf within the middle reaches the root too
	leaf->GetTransformComponent()->SetPosition(Vector3D(0.0f, 0.0f, -10.0f));
	Frame();
	CheckBox(root->GetHierarchyBounds(), Vector3D(-2.0f, -2.0f, -2.0f), Vector3D(2.0f, 2.0f, 2.0f));
}

TEST_CASE_FIXTURE(BoundedObjects, "Moving a child to another parent updates both")
{
	GameObjectPtr first = Add("First", k_unitBox, Vector3D(-10.0f, 0.0f, 0.0f));
	GameObjectPtr second = Add("Second", k_unitBox, Vector3D(10.0f, 0.0f, 0.0f));
	GameObjectPtr child = Add("Child", k_unitBox, Vector3D(0.0f, 5.0f, 0.0f), first);
	Frame();
	CheckBox(first->GetHierarchyBounds(), Vector3D(-10.5f, -0.5f, -0.5f), Vector3D(-9.5f, 5.5f, 0.5f));
	CheckBox(second->GetHierarchyBounds(), Vector3D(9.5f, -0.5f, -0.5f), Vector3D(10.5f, 0.5f, 0.5f));

	child->DetachFromParent();
	child->AttachTo(second);
	Frame();
	CheckBox(first->GetHierarchyBounds(), Vector3D(-10.5f, -0.5f, -0.5f), Vector3D(-9.5f, 0.5f, 0.5f));
	CheckBox(second->GetHierarchyBounds(), Vector3D(9.5f, -0.5f, -0.5f), Vector3D(10.5f, 5.5f, 0.5f));
}

TEST_CASE_FIXTURE(BoundedObjects, "Adding or removing a component changes the extents")
{
	GameObjectPtr object = Add("Object", BoundingBox(), Vector3D(1.0f, 2.0f, 3.0f));
	Frame();
	CHECK(object->GetHierarchyBounds().IsEmpty());

	object->AddComponent<BoundsTestComponent>()->SetBounds(k_unitBox);
	CheckBox(object->GetHierarchyBounds(), Vector3D(0.5f, 1.5f, 2.5f), Vector3D(1.5f, 2.5f, 3.5f));

	object->RemoveComponent<BoundsTestComponent>();
	CHECK(object->GetHierarchyBounds().IsEmpty());
}

TEST_CASE_FIXTURE(BoundedObjects, "What takes up no space doesn't count, wherever it is")
{
	GameObjectPtr parent = Add("Parent", k_unitBox, Vector3D());
	GameObjectPtr empty = Add("Empty", BoundingBox(), Vector3D(1000.0f, 0.0f, 0.0f), parent);
	Frame();
	CheckBox(parent->GetHierarchyBounds(), Vector3D(-0.5f, -0.5f, -0.5f), Vector3D(0.5f, 0.5f, 0.5f));
	CHECK(empty->GetHierarchyBounds().IsEmpty());

	empty->GetTransformComponent()->SetPosition(Vector3D(-1000.0f, 0.0f, 0.0f));
	Frame();
	CheckBox(parent->GetHierarchyBounds(), Vector3D(-0.5f, -0.5f, -0.5f), Vector3D(0.5f, 0.5f, 0.5f));

	GameObjectPtr nothing = Add("Nothing", BoundingBox(), Vector3D());
	Add("NothingEither", BoundingBox(), Vector3D(5.0f, 0.0f, 0.0f), nothing);
	Frame();
	CHECK(nothing->GetHierarchyBounds().IsEmpty());
}

TEST_CASE_FIXTURE(BoundedObjects, "An object taken out of the scene no longer counts toward its parent")
{
	GameObjectPtr parent = Add("Parent", k_unitBox, Vector3D());
	GameObjectPtr child = Add("Child", k_unitBox, Vector3D(5.0f, 0.0f, 0.0f), parent);
	Frame();
	CheckBox(parent->GetHierarchyBounds(), Vector3D(-0.5f, -0.5f, -0.5f), Vector3D(5.5f, 0.5f, 0.5f));

	Scene.RemoveGameObject(child);
	CheckBox(parent->GetHierarchyBounds(), Vector3D(-0.5f, -0.5f, -0.5f), Vector3D(0.5f, 0.5f, 0.5f));
}
