#pragma once

#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObject/GameObjectComponent.h>
#include <Game/World/GameObject/Properties/PropertyBuilder.h>

#include <Utils/Math/Vector3D.h>

// A component with one property of each kind tools treat differently, for tests of the property system and of
// what copying takes
class TestComponent final : public GameObjectComponent<TestComponent>
{
public:
	float GetSpeed() const { return m_speed; }
	void SetSpeed(float speed) { m_speed = speed; }

	bool IsDrawingDebug() const { return m_drawDebug; }
	void SetDrawDebug(bool draw) { m_drawDebug = draw; }

	bool IsLeader() const { return m_isLeader; }
	void SetLeader(bool isLeader) { m_isLeader = isLeader; }

	int GetSerial() const { return m_serial; }
	void SetSerial(int serial) { m_serial = serial; }

	Vector3D GetDoubledSize() const { return Vector3D(m_speed * 2.0f, 0.0f, 0.0f); }

	static void DescribeProperties(PropertyBuilder<TestComponent>& properties)
	{
		properties.Float("Speed", &TestComponent::GetSpeed, &TestComponent::SetSpeed).Range(0.0f, 10.0f).ToolTip("Metres per second");
		properties.Bool("Draw Debug", &TestComponent::IsDrawingDebug, &TestComponent::SetDrawDebug).EditorOnly();
		properties.Bool("Leader", &TestComponent::IsLeader, &TestComponent::SetLeader).Exclusive();
		properties.Int("Serial", &TestComponent::GetSerial, &TestComponent::SetSerial).ReadOnly();
		properties.Vector3("Doubled Size", &TestComponent::GetDoubledSize);
	}

	// Indices into the description, in DescribeProperties' order
	enum Property : size_t { Speed, DrawDebug, Leader, Serial, DoubledSize, Count };

private:
	float m_speed = 1.0f;
	bool m_drawDebug = false;
	bool m_isLeader = false;
	int m_serial = 0;
};

// A component type with no DescribeProperties
class UndescribedTestComponent final : public GameObjectComponent<UndescribedTestComponent>
{
};

// Takes up a box a test gives it, and counts how often its object asks for it (GameObject::GetHierarchyBounds asks
// each component when it works its extents out again), so a test can tell when an object's extents were reused
class BoundsTestComponent final : public GameObjectComponent<BoundsTestComponent>
{
public:
	void SetBounds(const BoundingBox& bounds)
	{
		m_bounds = bounds;
		GetOwner()->MarkHierarchyBoundsChanged();
	}

	BoundingBox GetLocalBounds() const override
	{
		++m_timesAsked;
		return m_bounds;
	}

	int GetTimesAsked() const { return m_timesAsked; }

private:
	BoundingBox m_bounds;
	mutable int m_timesAsked = 0;
};

// Registers the test component types, the way the engine registers its own at startup. Safe to call from every test.
inline void RegisterTestComponents()
{
	static bool s_isRegistered = false;
	if (!s_isRegistered)
	{
		REGISTER_GAMEOBJECTCOMPONENT(TestComponent);
		REGISTER_GAMEOBJECTCOMPONENT(UndescribedTestComponent);
		REGISTER_GAMEOBJECTCOMPONENT(BoundsTestComponent);
		s_isRegistered = true;
	}
}
