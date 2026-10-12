#pragma once

#include <Game/World/GameObject/Properties/PropertyDescriptor.h>

#include <cstddef>
#include <type_traits>
#include <vector>

namespace PropertyBuilderDetail
{
	template <typename TSetter>
	struct SetterTraits;

	template <typename TComponent, typename TArg>
	struct SetterTraits<void (TComponent::*)(TArg)>
	{
		using Component = TComponent;
		using Value = std::decay_t<TArg>;
	};
}

// Collects a component type's PropertyDescriptors from its getters and setters. A component describes its
// properties in a static DescribeProperties(PropertyBuilder<ComponentType>&), which ComponentPropertyRegistry
// calls once, when the component type is registered:
//
//   void CameraComponent::DescribeProperties(PropertyBuilder<CameraComponent>& properties)
//   {
//       properties.Float("Field Of View", &CameraComponent::GetFOV, &CameraComponent::SetFOV).Range(1.0f, 179.0f);
//       properties.Float("Aspect Ratio", &CameraComponent::GetAspectRatio);   // no setter: read-only
//   }
//
// Setters go through the component's own setter, so whatever it does on a change (e.g. marking the transform dirty)
// still happens. Leaving the setter out makes the property read-only.
template <typename TComponent>
class PropertyBuilder
{
private:
	// Fixed getter types rather than deduced ones, so an overloaded getter (e.g. const and non-const GetPosition)
	// resolves to its const version without a cast
	template <typename TValue>
	using ValueGetter = TValue (TComponent::*)() const;
	template <typename TValue>
	using RefGetter = const TValue& (TComponent::*)() const;

public:
	explicit PropertyBuilder(std::vector<PropertyDescriptor>& properties)
		: m_properties(properties) {}

public:
	template <typename TSetter = std::nullptr_t>
	PropertyDescriptor& Bool(const char* name, ValueGetter<bool> getter, TSetter setter = nullptr)
	{
		return Add<bool>(name, EPropertyType::Bool, getter, setter);
	}

	template <typename TSetter = std::nullptr_t>
	PropertyDescriptor& Int(const char* name, ValueGetter<int> getter, TSetter setter = nullptr)
	{
		return Add<int>(name, EPropertyType::Int, getter, setter);
	}

	template <typename TSetter = std::nullptr_t>
	PropertyDescriptor& Float(const char* name, ValueGetter<float> getter, TSetter setter = nullptr)
	{
		return Add<float>(name, EPropertyType::Float, getter, setter);
	}

	template <typename TSetter = std::nullptr_t>
	PropertyDescriptor& Vector3(const char* name, RefGetter<Vector3D> getter, TSetter setter = nullptr)
	{
		return Add<Vector3D>(name, EPropertyType::Vector3, getter, setter);
	}

	template <typename TSetter = std::nullptr_t>
	PropertyDescriptor& Vector3(const char* name, ValueGetter<Vector3D> getter, TSetter setter = nullptr)
	{
		return Add<Vector3D>(name, EPropertyType::Vector3, getter, setter);
	}

	template <typename TSetter = std::nullptr_t>
	PropertyDescriptor& Vector4(const char* name, RefGetter<Vector4D> getter, TSetter setter = nullptr)
	{
		return Add<Vector4D>(name, EPropertyType::Vector4, getter, setter);
	}

	template <typename TSetter = std::nullptr_t>
	PropertyDescriptor& Vector4(const char* name, ValueGetter<Vector4D> getter, TSetter setter = nullptr)
	{
		return Add<Vector4D>(name, EPropertyType::Vector4, getter, setter);
	}

	// An RGBA colour with components in [0, 1]
	template <typename TSetter = std::nullptr_t>
	PropertyDescriptor& Colour(const char* name, RefGetter<Vector4D> getter, TSetter setter = nullptr)
	{
		return Vector4(name, getter, setter).As(EPropertyWidget::Colour).Range(0.0f, 1.0f);
	}

	template <typename TSetter = std::nullptr_t>
	PropertyDescriptor& Colour(const char* name, ValueGetter<Vector4D> getter, TSetter setter = nullptr)
	{
		return Vector4(name, getter, setter).As(EPropertyWidget::Colour).Range(0.0f, 1.0f);
	}

	template <typename TSetter = std::nullptr_t>
	PropertyDescriptor& String(const char* name, RefGetter<std::string> getter, TSetter setter = nullptr)
	{
		return Add<std::string>(name, EPropertyType::String, getter, setter);
	}

	template <typename TSetter = std::nullptr_t>
	PropertyDescriptor& String(const char* name, ValueGetter<std::string> getter, TSetter setter = nullptr)
	{
		return Add<std::string>(name, EPropertyType::String, getter, setter);
	}

	// A path to an asset file; describe what to pick with .Asset(description, directory, filter)
	template <typename TSetter = std::nullptr_t>
	PropertyDescriptor& AssetPath(const char* name, RefGetter<Filepath> getter, TSetter setter = nullptr)
	{
		return Add<Filepath>(name, EPropertyType::AssetPath, getter, setter);
	}

	template <typename TSetter = std::nullptr_t>
	PropertyDescriptor& AssetPath(const char* name, ValueGetter<Filepath> getter, TSetter setter = nullptr)
	{
		return Add<Filepath>(name, EPropertyType::AssetPath, getter, setter);
	}

private:
	template <typename TValue, typename TGetter, typename TSetter>
	PropertyDescriptor& Add(const char* name, EPropertyType type, TGetter getter, TSetter setter)
	{
		PropertyDescriptor descriptor;
		descriptor.Name = name;
		descriptor.Type = type;
		descriptor.Get = [getter](const GameObjectComponentBase& component) -> PropertyValue
			{
				return PropertyValue(TValue((static_cast<const TComponent&>(component).*getter)()));
			};

		if constexpr (!std::is_same_v<TSetter, std::nullptr_t>)
		{
			using Traits = PropertyBuilderDetail::SetterTraits<TSetter>;
			static_assert(std::is_same_v<typename Traits::Component, TComponent>, "The setter must be a member of the component being described");
			static_assert(std::is_same_v<typename Traits::Value, TValue>, "The setter's parameter type doesn't match the property's type");

			descriptor.Set = [setter](GameObjectComponentBase& component, const PropertyValue& value)
				{
					(static_cast<TComponent&>(component).*setter)(std::get<TValue>(value));
				};
		}

		m_properties.push_back(std::move(descriptor));
		return m_properties.back();
	}

private:
	std::vector<PropertyDescriptor>& m_properties;
};
