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

TEST_CASE("An editable Array's elements are described by the component and keyed")
{
	RegisterTestComponents();
	const std::vector<PropertyDescriptor>& properties = *ComponentPropertyRegistry::GetProperties(TestComponent::GetID());
	const PropertyDescriptor& parts = properties[TestComponent::Parts];
	REQUIRE(parts.Type == EPropertyType::Array);
	CHECK(parts.HasEditableElements());
	REQUIRE(parts.ElementType != nullptr);
	CHECK(parts.ElementType->Properties.size() == TestComponent::PartPropertyCount);

	TestComponent component;
	component.SetPartCount(2);
	void* const object = ComponentPropertyRegistry::GetDescribedObject(component);

	// Reading gives each element's key, and no way to write it
	const std::vector<ObjectRef> read = std::get<std::vector<ObjectRef>>(parts.Get(object));
	REQUIRE(read.size() == 2);
	CHECK(read[1].Key == "Part1");
	CHECK(read[1].Type == parts.ElementType.get());
	CHECK(read[1].MutableObject == nullptr);

	const std::vector<ObjectRef> writable = std::get<std::vector<ObjectRef>>(parts.GetMutable(object));
	REQUIRE(writable.size() == 2);
	CHECK(writable[1].MutableObject == &component.GetParts()[1]);
}

TEST_CASE("A path leads to a component's own property, or into an editable Array's element by its key")
{
	RegisterTestComponents();
	TestComponent component;
	component.SetPartCount(3);

	const ComponentPropertyRegistry::ResolvedProperty speed = ComponentPropertyRegistry::Resolve(component, PropertyPath(TestComponent::Speed));
	REQUIRE(speed.IsValid());
	CHECK(speed.Property->Name == "Speed");
	speed.Property->Set(speed.Object, PropertyValue(6.0f));
	CHECK(component.GetSpeed() == 6.0f);

	const PropertyPath weight = PropertyPath(TestComponent::Parts).Nested("Part1", TestComponent::PartWeight);
	const ComponentPropertyRegistry::ResolvedProperty resolved = ComponentPropertyRegistry::Resolve(component, weight);
	REQUIRE(resolved.IsValid());
	CHECK(resolved.Property->Name == "Weight");
	resolved.Property->Set(resolved.Object, PropertyValue(5.0f));
	CHECK(component.GetParts()[1].GetWeight() == 5.0f);
	CHECK(component.GetParts()[0].GetWeight() == 1.0f);
}

TEST_CASE("A path leads nowhere once its element is gone, or through anything but an editable Array")
{
	RegisterTestComponents();
	TestComponent component;
	component.SetPartCount(2);

	const PropertyPath secondWeight = PropertyPath(TestComponent::Parts).Nested("Part1", TestComponent::PartWeight);
	CHECK(ComponentPropertyRegistry::Resolve(component, secondWeight).IsValid());

	component.SetPartCount(1);
	CHECK_FALSE(ComponentPropertyRegistry::Resolve(component, secondWeight).IsValid());

	CHECK_FALSE(ComponentPropertyRegistry::Resolve(component, PropertyPath(TestComponent::Speed).Nested("Part0", 0)).IsValid());
	CHECK_FALSE(ComponentPropertyRegistry::Resolve(component, PropertyPath(TestComponent::Count)).IsValid());
	CHECK_FALSE(ComponentPropertyRegistry::Resolve(component, PropertyPath()).IsValid());
}

TEST_CASE("Paths are equal when they lead through the same properties and elements")
{
	const PropertyPath weight = PropertyPath(TestComponent::Parts).Nested("Part1", TestComponent::PartWeight);
	CHECK(weight == PropertyPath(TestComponent::Parts).Nested("Part1", TestComponent::PartWeight));
	CHECK(weight != PropertyPath(TestComponent::Parts).Nested("Part0", TestComponent::PartWeight));
	CHECK(weight != PropertyPath(TestComponent::Parts).Nested("Part1", TestComponent::PartLocked));
	CHECK(weight != PropertyPath(TestComponent::Parts));
	CHECK(weight.IsNested());
	CHECK_FALSE(PropertyPath(TestComponent::Speed).IsNested());
	CHECK(PropertyPath(TestComponent::Speed) == PropertyPath(TestComponent::Speed));
}

TEST_CASE("An overridable value reports and resets its override")
{
	RegisterTestComponents();
	const std::vector<PropertyDescriptor>& properties = *ComponentPropertyRegistry::GetProperties(TestComponent::GetID());
	const PropertyDescriptor& mass = properties[TestComponent::Parts].ElementType->Properties[TestComponent::PartMass];
	REQUIRE(mass.IsOverridable());
	CHECK(mass.IsEditable());
	CHECK(mass.Group == "Physics");

	TestPart part("Part0");
	CHECK_FALSE(mass.IsOverridden(&part));
	CHECK(std::get<float>(mass.Get(&part)) == TestPart::k_baseMass);

	mass.Set(&part, PropertyValue(5.0f));
	CHECK(mass.IsOverridden(&part));
	CHECK(std::get<float>(mass.Get(&part)) == 5.0f);

	mass.Reset(&part);
	CHECK_FALSE(mass.IsOverridden(&part));
	CHECK(std::get<float>(mass.Get(&part)) == TestPart::k_baseMass);

	// Nothing else is overridable
	CHECK_FALSE(properties[TestComponent::Speed].IsOverridable());
}

TEST_CASE("A value can be disabled for a reason, depending on the object")
{
	RegisterTestComponents();
	const std::vector<PropertyDescriptor>& properties = *ComponentPropertyRegistry::GetProperties(TestComponent::GetID());
	const PropertyDescriptor& mass = properties[TestComponent::Parts].ElementType->Properties[TestComponent::PartMass];
	REQUIRE(mass.IsEnabled);

	TestPart part("Part0");
	CHECK(mass.IsEnabled(&part));
	part.SetLocked(true);
	CHECK_FALSE(mass.IsEnabled(&part));
	CHECK(mass.DisabledReason == "Unlock the part to change its mass");

	// Without a condition, it's always enabled
	CHECK_FALSE(properties[TestComponent::Speed].IsEnabled);
}

TEST_CASE("An enum is its option's index, with the options' names")
{
	RegisterTestComponents();
	const std::vector<PropertyDescriptor>& properties = *ComponentPropertyRegistry::GetProperties(TestComponent::GetID());
	const PropertyDescriptor& shape = properties[TestComponent::Parts].ElementType->Properties[TestComponent::PartShape];
	REQUIRE(shape.Type == EPropertyType::Enum);
	REQUIRE(shape.EnumNames.size() == 3);
	CHECK(shape.EnumNames[2] == "Capsule");

	TestPart part("Part0");
	CHECK(std::get<int>(shape.Get(&part)) == 0);
	shape.Set(&part, PropertyValue(2));
	CHECK(part.GetShape() == TestPart::EShape::Capsule);
	CHECK(std::get<int>(shape.Get(&part)) == 2);
}
