#pragma once

#include <Game/World/GameObject/Properties/PropertyDescriptor.h>

#include <cstddef>
#include <memory>
#include <type_traits>
#include <vector>

template <typename TObject>
class PropertyBuilder;

// The description of T, built from T::DescribeProperties(PropertyBuilder<T>&) the first time it's asked for
template <typename T>
const TypeDescription& DescribeType();

// A reference to object for nesting it in another type's properties (PropertyBuilder::Object, ::Array). Invalid if
// object is null.
template <typename T>
ObjectRef MakeObjectRef(const T* object, std::string label = std::string())
{
	ObjectRef ref;
	if (object != nullptr)
	{
		ref.Type = &DescribeType<T>();
		ref.Object = object;
		ref.Label = std::move(label);
	}
	return ref;
}

// References to each element of objects, e.g. for an Array property over a std::vector
template <typename T>
std::vector<ObjectRef> MakeObjectRefs(const std::vector<T>& objects)
{
	std::vector<ObjectRef> refs;
	refs.reserve(objects.size());
	for (const T& object : objects)
	{
		refs.push_back(MakeObjectRef(&object));
	}
	return refs;
}

namespace PropertyBuilderDetail
{
	template <typename TSetter>
	struct SetterTraits;

	template <typename TObject, typename TArg>
	struct SetterTraits<void (TObject::*)(TArg)>
	{
		using Object = TObject;
		using Value = std::decay_t<TArg>;
	};

	// The EPropertyType of a value type, for Value<T>()
	template <typename TValue> struct TypeOf;
	template <> struct TypeOf<bool> { static constexpr EPropertyType Type = EPropertyType::Bool; };
	template <> struct TypeOf<int> { static constexpr EPropertyType Type = EPropertyType::Int; };
	template <> struct TypeOf<float> { static constexpr EPropertyType Type = EPropertyType::Float; };
	template <> struct TypeOf<Vector3D> { static constexpr EPropertyType Type = EPropertyType::Vector3; };
	template <> struct TypeOf<Vector4D> { static constexpr EPropertyType Type = EPropertyType::Vector4; };
	template <> struct TypeOf<std::string> { static constexpr EPropertyType Type = EPropertyType::String; };
	template <> struct TypeOf<Filepath> { static constexpr EPropertyType Type = EPropertyType::AssetPath; };
	template <> struct TypeOf<PropertyTexture> { static constexpr EPropertyType Type = EPropertyType::Texture; };

	template <typename T> const T* ToPointer(const T& value) { return &value; }
	template <typename T> const T* ToPointer(const T* value) { return value; }
	template <typename T> const T* ToPointer(const std::shared_ptr<const T>& value) { return value.get(); }
}

// Describes a type's properties from its getters and setters. A type describes itself in a static
// DescribeProperties(PropertyBuilder<Type>&), which DescribeType<Type>() calls the first time it's needed (for a
// component type, when it's registered with REGISTER_GAMEOBJECTCOMPONENT):
//
//   void CameraComponent::DescribeProperties(PropertyBuilder<CameraComponent>& properties)
//   {
//       properties.Float("Field Of View", &CameraComponent::GetFOV, &CameraComponent::SetFOV).Range(1.0f, 179.0f);
//       properties.Float("Aspect Ratio", &CameraComponent::GetAspectRatio);   // no setter: computed
//       properties.Object<MaterialInstance>("Material", &MeshGeometry::GetMaterial);   // a nested described type
//       properties.Value<std::string>("Shader", [](const MaterialInstance& m) { return ...; });   // computed
//   }
//
// Setters go through the type's own setter, so whatever it does on a change (e.g. marking the transform dirty)
// still happens. Leaving the setter out makes the property computed: worked out from the others, so tools show it
// but never write or copy it. State that tools mustn't edit but that belongs to the object keeps its setter and is
// marked .ReadOnly() instead.
template <typename TObject>
class PropertyBuilder
{
private:
	// Fixed getter types rather than deduced ones, so an overloaded getter (e.g. const and non-const GetPosition)
	// resolves to its const version without a cast
	template <typename TValue>
	using ValueGetter = TValue (TObject::*)() const;
	template <typename TValue>
	using RefGetter = const TValue& (TObject::*)() const;
	template <typename TValue>
	using PointerGetter = const TValue* (TObject::*)() const;
	template <typename TValue>
	using SharedGetter = std::shared_ptr<const TValue> (TObject::*)() const;

public:
	explicit PropertyBuilder(TypeDescription& description)
		: m_description(description) {}

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

	// A nested described object (TValue must have a DescribeProperties), from a getter returning a reference,
	// pointer or shared_ptr to it. Null pointers show as empty. Read-only.
	template <typename TValue>
	PropertyDescriptor& Object(const char* name, RefGetter<TValue> getter) { return AddObject<TValue>(name, getter); }
	template <typename TValue>
	PropertyDescriptor& Object(const char* name, PointerGetter<TValue> getter) { return AddObject<TValue>(name, getter); }
	template <typename TValue>
	PropertyDescriptor& Object(const char* name, SharedGetter<TValue> getter) { return AddObject<TValue>(name, getter); }

	// A list of nested described objects. getter takes the object and returns a std::vector<ObjectRef>, e.g. from
	// MakeObjectRefs. Read-only.
	template <typename TGetter>
	PropertyDescriptor& Array(const char* name, TGetter getter)
	{
		PropertyDescriptor& descriptor = AddDescriptor(name, EPropertyType::Array);
		descriptor.Get = [getter](const void* object) -> PropertyValue
			{
				return PropertyValue(std::vector<ObjectRef>(getter(*static_cast<const TObject*>(object))));
			};
		return descriptor;
	}

	// A list of nested objects whose properties tools may set, described here by the component rather than by the
	// elements' own type. getter takes the object and returns a const std::vector<TElement>&; mutableGetter takes it
	// non-const and returns a std::vector<TElement>&; keyOf takes an element and returns its ObjectRef::Key, which must
	// stay the same while the element does (e.g. a name), so a PropertyPath still finds it after the list changes;
	// describe takes a PropertyBuilder<TElement>& and describes the elements' properties, once.
	template <typename TElement, typename TGetter, typename TMutableGetter, typename TKeyOf, typename TDescribe>
	PropertyDescriptor& Array(const char* name, TGetter getter, TMutableGetter mutableGetter, TKeyOf keyOf, TDescribe describe)
	{
		std::shared_ptr<TypeDescription> elementType = std::make_shared<TypeDescription>();
		PropertyBuilder<TElement> elementBuilder(*elementType);
		describe(elementBuilder);
		const TypeDescription* const type = elementType.get();

		PropertyDescriptor& descriptor = AddDescriptor(name, EPropertyType::Array);
		descriptor.ElementType = std::move(elementType);
		descriptor.Get = [getter, keyOf, type](const void* object) -> PropertyValue
			{
				std::vector<ObjectRef> refs;
				for (const TElement& element : getter(*static_cast<const TObject*>(object)))
				{
					ObjectRef& ref = refs.emplace_back();
					ref.Type = type;
					ref.Object = &element;
					ref.Key = keyOf(element);
				}
				return PropertyValue(std::move(refs));
			};
		descriptor.GetMutable = [mutableGetter, keyOf, type](void* object) -> PropertyValue
			{
				std::vector<ObjectRef> refs;
				for (TElement& element : mutableGetter(*static_cast<TObject*>(object)))
				{
					ObjectRef& ref = refs.emplace_back();
					ref.Type = type;
					ref.Object = &element;
					ref.MutableObject = &element;
					ref.Key = keyOf(element);
				}
				return PropertyValue(std::move(refs));
			};
		return descriptor;
	}

	// A read-only value worked out by getter, which takes the object and returns a TValue; for values with no
	// getter of their own (e.g. a texture preview, or a name looked up elsewhere)
	template <typename TValue, typename TGetter>
	PropertyDescriptor& Value(const char* name, TGetter getter)
	{
		PropertyDescriptor& descriptor = AddDescriptor(name, PropertyBuilderDetail::TypeOf<TValue>::Type);
		descriptor.Get = [getter](const void* object) -> PropertyValue
			{
				return PropertyValue(TValue(getter(*static_cast<const TObject*>(object))));
			};
		return descriptor;
	}

	// What to call an instance in a tree (e.g. a sub-mesh's name); getter takes the object and returns a string
	template <typename TGetter>
	void DisplayName(TGetter getter)
	{
		m_description.GetDisplayName = [getter](const void* object) -> std::string
			{
				return std::string(getter(*static_cast<const TObject*>(object)));
			};
	}

private:
	PropertyDescriptor& AddDescriptor(const char* name, EPropertyType type)
	{
		PropertyDescriptor descriptor;
		descriptor.Name = name;
		descriptor.Type = type;
		m_description.Properties.push_back(std::move(descriptor));
		return m_description.Properties.back();
	}

	template <typename TValue, typename TGetter, typename TSetter>
	PropertyDescriptor& Add(const char* name, EPropertyType type, TGetter getter, TSetter setter)
	{
		PropertyDescriptor& descriptor = AddDescriptor(name, type);
		descriptor.Get = [getter](const void* object) -> PropertyValue
			{
				return PropertyValue(TValue((static_cast<const TObject*>(object)->*getter)()));
			};

		if constexpr (!std::is_same_v<TSetter, std::nullptr_t>)
		{
			using Traits = PropertyBuilderDetail::SetterTraits<TSetter>;
			static_assert(std::is_same_v<typename Traits::Object, TObject>, "The setter must be a member of the type being described");
			static_assert(std::is_same_v<typename Traits::Value, TValue>, "The setter's parameter type doesn't match the property's type");

			descriptor.Set = [setter](void* object, const PropertyValue& value)
				{
					(static_cast<TObject*>(object)->*setter)(std::get<TValue>(value));
				};
		}

		return descriptor;
	}

	template <typename TValue, typename TGetter>
	PropertyDescriptor& AddObject(const char* name, TGetter getter)
	{
		PropertyDescriptor& descriptor = AddDescriptor(name, EPropertyType::Object);
		descriptor.Get = [getter](const void* object) -> PropertyValue
			{
				const TValue* const value = PropertyBuilderDetail::ToPointer((static_cast<const TObject*>(object)->*getter)());
				return PropertyValue(MakeObjectRef(value));
			};
		return descriptor;
	}

private:
	TypeDescription& m_description;
};

template <typename T>
const TypeDescription& DescribeType()
{
	// Built once, the first time it's needed; thread-safe as a function-local static
	static const TypeDescription s_description = []()
		{
			TypeDescription description;
			PropertyBuilder<T> builder(description);
			T::DescribeProperties(builder);
			return description;
		}();
	return s_description;
}
