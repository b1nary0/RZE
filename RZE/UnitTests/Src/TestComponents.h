#pragma once

#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObject/GameObjectComponent.h>
#include <Game/World/GameObject/Properties/PropertyBuilder.h>

#include <Utils/Math/Vector3D.h>

#include <optional>
#include <string>
#include <vector>

// An element of TestComponent's editable Parts list. Described by TestComponent, not by itself.
class TestPart
{
public:
	enum class EShape { Box, Sphere, Capsule };

	// What Mass is when the part doesn't override it
	static constexpr float k_baseMass = 2.0f;

	explicit TestPart(const std::string& name) : m_name(name) {}

	const std::string& GetName() const { return m_name; }

	float GetWeight() const { return m_weight; }
	void SetWeight(float weight) { m_weight = weight; }

	bool IsLocked() const { return m_isLocked; }
	void SetLocked(bool isLocked) { m_isLocked = isLocked; }

	// Overridable: k_baseMass until set
	float GetMass() const { return m_massOverride.value_or(k_baseMass); }
	void SetMass(float mass) { m_massOverride = mass; }
	bool IsMassOverridden() const { return m_massOverride.has_value(); }
	void ResetMass() { m_massOverride.reset(); }

	EShape GetShape() const { return m_shape; }
	void SetShape(EShape shape) { m_shape = shape; }

private:
	std::string m_name;
	float m_weight = 1.0f;
	bool m_isLocked = false;
	std::optional<float> m_massOverride;
	EShape m_shape = EShape::Box;
};

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

	// Setting the count makes parts "Part0", "Part1", ..., like a mesh making its material slots: copies set it
	// before the parts' own properties
	int GetPartCount() const { return static_cast<int>(m_parts.size()); }
	void SetPartCount(int count)
	{
		m_parts.clear();
		for (int partIndex = 0; partIndex < count; ++partIndex)
		{
			m_parts.emplace_back("Part" + std::to_string(partIndex));
		}
	}

	std::vector<TestPart>& GetParts() { return m_parts; }
	const std::vector<TestPart>& GetParts() const { return m_parts; }

	static void DescribeProperties(PropertyBuilder<TestComponent>& properties)
	{
		properties.Float("Speed", &TestComponent::GetSpeed, &TestComponent::SetSpeed).Range(0.0f, 10.0f).ToolTip("Metres per second");
		properties.Bool("Draw Debug", &TestComponent::IsDrawingDebug, &TestComponent::SetDrawDebug).EditorOnly();
		properties.Bool("Leader", &TestComponent::IsLeader, &TestComponent::SetLeader).Exclusive();
		properties.Int("Serial", &TestComponent::GetSerial, &TestComponent::SetSerial).ReadOnly();
		properties.Vector3("Doubled Size", &TestComponent::GetDoubledSize);
		properties.Int("Part Count", &TestComponent::GetPartCount, &TestComponent::SetPartCount);
		properties.Array<TestPart>("Parts",
			[](const TestComponent& component) -> const std::vector<TestPart>& { return component.GetParts(); },
			[](TestComponent& component) -> std::vector<TestPart>& { return component.GetParts(); },
			[](const TestPart& part) { return part.GetName(); },
			[](PropertyBuilder<TestPart>& part)
			{
				part.DisplayName([](const TestPart& testPart) { return testPart.GetName(); });
				part.Float("Weight", &TestPart::GetWeight, &TestPart::SetWeight);
				part.Bool("Locked", &TestPart::IsLocked, &TestPart::SetLocked).EditorOnly();
				part.Value<float>("Mass",
					[](const TestPart& testPart) { return testPart.GetMass(); },
					[](TestPart& testPart, const float& mass) { testPart.SetMass(mass); })
					.Overridable<TestPart>(
						[](const TestPart& testPart) { return testPart.IsMassOverridden(); },
						[](TestPart& testPart) { testPart.ResetMass(); })
					.EnabledIf<TestPart>([](const TestPart& testPart) { return !testPart.IsLocked(); }, "Unlock the part to change its mass")
					.InGroup("Physics");
				part.Enum<TestPart::EShape>("Shape", { "Box", "Sphere", "Capsule" },
					[](const TestPart& testPart) { return testPart.GetShape(); },
					[](TestPart& testPart, TestPart::EShape shape) { testPart.SetShape(shape); })
					.InGroup("Physics");
			});
	}

	// Indices into the description, in DescribeProperties' order
	enum Property : size_t { Speed, DrawDebug, Leader, Serial, DoubledSize, PartCount, Parts, Count };
	// Indices into a part's description
	enum PartProperty : size_t { PartWeight, PartLocked, PartMass, PartShape, PartPropertyCount };

private:
	float m_speed = 1.0f;
	bool m_drawDebug = false;
	bool m_isLeader = false;
	int m_serial = 0;
	std::vector<TestPart> m_parts;
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
