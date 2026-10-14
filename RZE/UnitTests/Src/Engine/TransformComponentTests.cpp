#include <doctest/doctest.h>

#include <RZE.h>

#include <Game/GameScene.h>
#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/TransformComponent.h>

#include <cmath>
#include <string>

// None of these update the scene: a world matrix is worked out when it's read

namespace
{
	void CheckPosition(const Matrix4x4& world, const Vector3D& expected)
	{
		const Vector3D position = world.GetPosition();
		for (int axis = 0; axis < 3; ++axis)
		{
			CHECK(position[axis] == doctest::Approx(expected[axis]).epsilon(0.0001));
		}
	}

	struct Objects
	{
		GameScene Scene;
		~Objects() { Scene.Unload(); }

		GameObjectPtr Add(const std::string& name, const Vector3D& position, GameObjectPtr parent = nullptr)
		{
			GameObjectPtr object = Scene.AddGameObject(name);
			if (parent != nullptr)
			{
				object->AttachTo(parent);
			}
			object->GetTransformComponent()->SetPosition(position);
			return object;
		}
	};
}

TEST_CASE("A transform made with a position is a root, placed there")
{
	const TransformComponent transform(Vector3D(1.0f, 2.0f, 3.0f), Vector3D(), Vector3D(1.0f));
	CHECK(transform.IsRoot());
	CheckPosition(transform.GetWorldMatrix(), Vector3D(1.0f, 2.0f, 3.0f));
}

TEST_CASE_FIXTURE(Objects, "A world matrix is right as soon as the transform is set, without an update")
{
	GameObjectPtr object = Add("Object", Vector3D(3.0f, 0.0f, 0.0f));
	CheckPosition(object->GetTransformComponent()->GetWorldMatrix(), Vector3D(3.0f, 0.0f, 0.0f));

	object->GetTransformComponent()->SetPosition(Vector3D(-4.0f, 5.0f, 6.0f));
	CheckPosition(object->GetTransformComponent()->GetWorldMatrix(), Vector3D(-4.0f, 5.0f, 6.0f));
}

TEST_CASE_FIXTURE(Objects, "A child's world matrix follows its parent's changes when it's read")
{
	GameObjectPtr parent = Add("Parent", Vector3D(10.0f, 0.0f, 0.0f));
	GameObjectPtr child = Add("Child", Vector3D(1.0f, 0.0f, 0.0f), parent);
	CheckPosition(child->GetTransformComponent()->GetWorldMatrix(), Vector3D(11.0f, 0.0f, 0.0f));

	parent->GetTransformComponent()->SetPosition(Vector3D(20.0f, 0.0f, 0.0f));
	CheckPosition(child->GetTransformComponent()->GetWorldMatrix(), Vector3D(21.0f, 0.0f, 0.0f));

	// A quarter turn of the parent about Y swings the child round from +X
	parent->GetTransformComponent()->SetRotation(Vector3D(0.0f, 90.0f, 0.0f));
	const Vector3D childPosition = child->GetTransformComponent()->GetWorldMatrix().GetPosition();
	CHECK(childPosition.X() == doctest::Approx(20.0f).epsilon(0.0001));
	CHECK(std::abs(childPosition.Z()) == doctest::Approx(1.0f).epsilon(0.0001));
}

TEST_CASE_FIXTURE(Objects, "Detaching a child leaves its position in the world rather than in the parent")
{
	GameObjectPtr parent = Add("Parent", Vector3D(10.0f, 0.0f, 0.0f));
	GameObjectPtr child = Add("Child", Vector3D(1.0f, 0.0f, 0.0f), parent);
	CheckPosition(child->GetTransformComponent()->GetWorldMatrix(), Vector3D(11.0f, 0.0f, 0.0f));

	child->DetachFromParent();
	CHECK(child->GetTransformComponent()->IsRoot());
	CheckPosition(child->GetTransformComponent()->GetWorldMatrix(), Vector3D(1.0f, 0.0f, 0.0f));
}
