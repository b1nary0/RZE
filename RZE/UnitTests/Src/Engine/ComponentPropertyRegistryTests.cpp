#include <doctest/doctest.h>

#include <RZE.h>

#include <TestComponents.h>

#include <Game/World/GameObject/Properties/ComponentPropertyRegistry.h>

#include <variant>
#include <vector>

TEST_CASE("A registered component's properties are described in DescribeProperties' order")
{
	RegisterTestComponents();

	const std::vector<PropertyDescriptor>* const properties = ComponentPropertyRegistry::GetProperties(TestComponent::GetID());
	REQUIRE(properties != nullptr);
	REQUIRE(properties->size() == TestComponent::Count);

	CHECK((*properties)[TestComponent::Speed].Name == "Speed");
	CHECK((*properties)[TestComponent::Speed].Type == EPropertyType::Float);
	CHECK((*properties)[TestComponent::DrawDebug].Type == EPropertyType::Bool);
	CHECK((*properties)[TestComponent::Serial].Type == EPropertyType::Int);
	CHECK((*properties)[TestComponent::DoubledSize].Type == EPropertyType::Vector3);
}

TEST_CASE("A component type without DescribeProperties has no description")
{
	RegisterTestComponents();

	CHECK(ComponentPropertyRegistry::GetDescription(UndescribedTestComponent::GetID()) == nullptr);
	CHECK(ComponentPropertyRegistry::GetProperties(UndescribedTestComponent::GetID()) == nullptr);

	UndescribedTestComponent component;
	CHECK(ComponentPropertyRegistry::GetDescribedObject(component) == nullptr);
}

TEST_CASE("Properties read and write the component through its getters and setters")
{
	RegisterTestComponents();
	const std::vector<PropertyDescriptor>& properties = *ComponentPropertyRegistry::GetProperties(TestComponent::GetID());

	TestComponent component;
	void* const object = ComponentPropertyRegistry::GetDescribedObject(component);
	REQUIRE(object != nullptr);

	properties[TestComponent::Speed].Set(object, PropertyValue(4.5f));
	CHECK(component.GetSpeed() == 4.5f);
	CHECK(std::get<float>(properties[TestComponent::Speed].Get(object)) == 4.5f);

	// A computed property follows the values it's worked out from
	CHECK(std::get<Vector3D>(properties[TestComponent::DoubledSize].Get(object)).X() == 9.0f);
}

TEST_CASE("Metadata set in DescribeProperties is kept")
{
	RegisterTestComponents();
	const std::vector<PropertyDescriptor>& properties = *ComponentPropertyRegistry::GetProperties(TestComponent::GetID());

	const PropertyDescriptor& speed = properties[TestComponent::Speed];
	CHECK(speed.HasRange());
	CHECK(speed.Min == 0.0f);
	CHECK(speed.Max == 10.0f);
	CHECK(speed.Tooltip == "Metres per second");

	CHECK(properties[TestComponent::DrawDebug].IsEditorOnly);
	CHECK(properties[TestComponent::Leader].IsExclusive);
	CHECK_FALSE(speed.IsEditorOnly);
	CHECK_FALSE(speed.IsExclusive);
}

TEST_CASE("Only properties with a setter that aren't ReadOnly are editable")
{
	RegisterTestComponents();
	const std::vector<PropertyDescriptor>& properties = *ComponentPropertyRegistry::GetProperties(TestComponent::GetID());

	CHECK(properties[TestComponent::Speed].IsEditable());

	// ReadOnly keeps its setter, so it can still be written by tools other than the Inspector (e.g. copying)
	const PropertyDescriptor& serial = properties[TestComponent::Serial];
	CHECK_FALSE(serial.IsEditable());
	CHECK(serial.Set != nullptr);

	// Computed: no setter at all
	const PropertyDescriptor& doubledSize = properties[TestComponent::DoubledSize];
	CHECK_FALSE(doubledSize.IsEditable());
	CHECK(doubledSize.Set == nullptr);
}
