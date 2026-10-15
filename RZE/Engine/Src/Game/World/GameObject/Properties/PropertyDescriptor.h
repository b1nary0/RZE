#pragma once

#include <Rendering/BufferHandle.h>

#include <Utils/Math/Vector2D.h>
#include <Utils/Math/Vector3D.h>
#include <Utils/Math/Vector4D.h>
#include <Utils/Platform/FilePath.h>

#include <functional>
#include <memory>
#include <string>
#include <variant>
#include <vector>

struct TypeDescription;

// A described object somewhere in memory: what it is (its TypeDescription) and where. Non-owning; only valid while
// the object is, which for a nested property's value means the frame it was read in.
struct ObjectRef
{
	const TypeDescription* Type = nullptr;
	const void* Object = nullptr;
	// The same object, for writing its properties; only set by an editable nested property's GetMutable
	void* MutableObject = nullptr;
	// Shown instead of the type's display name, e.g. a material's texture slot ("Diffuse")
	std::string Label;
	// Identifies an Array element from one read to the next (e.g. a material slot's name), so a PropertyPath still
	// finds it after the list changes; empty means its index (see ComponentPropertyRegistry::GetElementKey)
	std::string Key;

	bool IsValid() const { return Type != nullptr && Object != nullptr; }
};

// An image to show, e.g. a texture asset or a render target. Only the [0, UVMax] corner of the texture is shown,
// at Size pixels.
struct PropertyTexture
{
	Rendering::TextureBuffer2DHandle Handle;
	Vector2D Size;
	Vector2D UVMax = Vector2D(1.0f, 1.0f);
};

// A property's value, type-erased so tools can read and write any described type's properties without knowing the
// type. The alternatives' order matches EPropertyType's, up to Texture; an Enum's value is an int.
using PropertyValue = std::variant<bool, int, float, Vector3D, Vector4D, std::string, Filepath, ObjectRef, std::vector<ObjectRef>, PropertyTexture>;

enum class EPropertyType
{
	Bool,
	Int,
	Float,
	Vector3,
	Vector4,
	String,
	// A Filepath to an asset file, picked with a file prompt; see PropertyDescriptor::Asset
	AssetPath,
	// Another described object, shown nested; read-only
	Object,
	// A list of described objects, each shown nested; editable if the property has GetMutable
	Array,
	// An image; read-only
	Texture,
	// One of a fixed set of named options (PropertyDescriptor::EnumNames), as an int: the option's index
	Enum
};

// A hint for how an editor should show the value; types without a matching widget fall back to their default
enum class EPropertyWidget
{
	Default,
	// Dragged with the mouse rather than typed
	Drag,
	Slider,
	// A Vector3 or Vector4 shown as an RGB(A) colour
	Colour
};

// One value on a described type: its name, type, how to read and write it, and how tools should present it.
// Built by PropertyBuilder; the metadata setters chain, e.g. .Range(1.0f, 179.0f).StepSize(0.5f).
struct PropertyDescriptor
{
	// object points at an instance of the described type
	using Getter = std::function<PropertyValue(const void* object)>;
	using Setter = std::function<void(void* object, const PropertyValue&)>;
	// For an editable Array: like Getter, but the refs' MutableObject is set
	using MutableGetter = std::function<PropertyValue(void* object)>;
	using ObjectQuery = std::function<bool(const void* object)>;
	using ObjectAction = std::function<void(void* object)>;
	using PreviewGetter = std::function<PropertyTexture(const void* object)>;

	std::string Name;
	EPropertyType Type = EPropertyType::Float;
	Getter Get;
	// Null for a computed property: one worked out from other properties (e.g. a mesh's size), which tools show
	// but never write, and copying skips since setting the others reproduces it. State that tools mustn't edit but
	// that still belongs to the object keeps its setter and is marked ReadOnly instead.
	Setter Set;
	// Only for an Array whose elements' properties tools may set (each through a PropertyPath); null otherwise
	MutableGetter GetMutable;
	// The elements' description, when the component describes them itself (PropertyBuilder::Array with a describe
	// function). Owned here so the refs' Type stays valid as long as the component type's description does.
	std::shared_ptr<const TypeDescription> ElementType;

	// Inclusive limits for numeric types; ignored when Min >= Max
	float Min = 0.0f;
	float Max = 0.0f;
	// Per step of an input's +/- buttons, or per pixel of a drag; 0 lets the editor choose
	float Step = 0.0f;
	// printf format for floats, e.g. "%.2f"; empty lets the editor choose
	std::string Format;
	std::string Tooltip;
	EPropertyWidget Widget = EPropertyWidget::Default;
	// For Bool properties that at most one object of the described type in the scene can have on, such as a
	// camera's Main Camera. The setter keeps it that way, turning it off for the others when it's turned on, so it's
	// only ever turned on: tools turn it off by turning it on for another object.
	bool IsExclusive = false;
	// A view setting rather than part of what's saved (e.g. drawing debug bounds): tools apply it without counting it
	// as a change to the scene, so it isn't undoable and doesn't mark the scene modified
	bool IsEditorOnly = false;
	// Shown but not editable in tools, though it has a setter, so it's still copied with the object
	bool IsReadOnly = false;

	// For AssetPath properties: the file prompt's title, the folder it starts in, and its filter (e.g. "*.meshasset")
	std::string AssetDescription;
	std::string AssetDirectory;
	std::string AssetFilter;
	// For AssetPath properties: a small image of the asset, from the object (e.g. its loaded texture); null for none
	PreviewGetter GetPreview;
	// For AssetPath properties: tools can clear it (set an empty Filepath)
	bool IsClearable = false;

	// For Enum properties: the options' names, in the order of their values (0, 1, 2, ...)
	std::vector<std::string> EnumNames;

	// For a value that comes from a base unless the object overrides it (e.g. a material slot's values over the
	// shared material's): whether it's overridden, and how to go back to the base. Tools mark overridden values and
	// can reset them, undo can return to "not overridden", and copies only take overridden values. Both null for
	// anything else.
	ObjectQuery IsOverridden;
	ObjectAction Reset;

	// Whether tools let it be changed at the moment (e.g. a material's opacity only matters when it's blended); null
	// means always. When it isn't, it's shown read-only, with DisabledReason as its tooltip.
	ObjectQuery IsEnabled;
	std::string DisabledReason;

	// Shown under a sub-heading of this name, with the properties next to it that share it; empty for none
	std::string Group;

	// Whether tools let the user change it
	bool IsEditable() const { return Set && !IsReadOnly; }
	// Whether tools can reach into its elements to set their properties
	bool HasEditableElements() const { return static_cast<bool>(GetMutable); }
	bool IsOverridable() const { return IsOverridden && Reset; }
	bool HasRange() const { return Min < Max; }

	PropertyDescriptor& Range(float min, float max) { Min = min; Max = max; return *this; }
	PropertyDescriptor& StepSize(float step) { Step = step; return *this; }
	PropertyDescriptor& DisplayFormat(const char* format) { Format = format; return *this; }
	PropertyDescriptor& ToolTip(const char* tooltip) { Tooltip = tooltip; return *this; }
	PropertyDescriptor& As(EPropertyWidget widget) { Widget = widget; return *this; }
	PropertyDescriptor& Exclusive() { IsExclusive = true; return *this; }
	PropertyDescriptor& EditorOnly() { IsEditorOnly = true; return *this; }
	PropertyDescriptor& ReadOnly() { IsReadOnly = true; return *this; }
	PropertyDescriptor& Asset(const char* description, const char* directory, const char* filter)
	{
		AssetDescription = description;
		AssetDirectory = directory;
		AssetFilter = filter;
		return *this;
	}
	PropertyDescriptor& Clearable() { IsClearable = true; return *this; }
	PropertyDescriptor& InGroup(const char* group) { Group = group; return *this; }

	// The typed metadata below takes functions of the described type, TObject, which must be given explicitly, e.g.
	// .Overridable<MaterialSlot>(...)

	// isOverridden takes the object and returns whether it overrides the value; reset takes it non-const and goes
	// back to the base. See IsOverridden.
	template <typename TObject, typename TIsOverridden, typename TReset>
	PropertyDescriptor& Overridable(TIsOverridden isOverridden, TReset reset)
	{
		IsOverridden = [isOverridden](const void* object) { return static_cast<bool>(isOverridden(*static_cast<const TObject*>(object))); };
		Reset = [reset](void* object) { reset(*static_cast<TObject*>(object)); };
		return *this;
	}

	// isEnabled takes the object and returns whether the value can be changed; reason says why it can't
	template <typename TObject, typename TIsEnabled>
	PropertyDescriptor& EnabledIf(TIsEnabled isEnabled, const char* reason)
	{
		IsEnabled = [isEnabled](const void* object) { return static_cast<bool>(isEnabled(*static_cast<const TObject*>(object))); };
		DisabledReason = reason;
		return *this;
	}

	// For AssetPath properties: getter takes the object and returns a PropertyTexture of the asset
	template <typename TObject, typename TGetter>
	PropertyDescriptor& Preview(TGetter getter)
	{
		GetPreview = [getter](const void* object) { return PropertyTexture(getter(*static_cast<const TObject*>(object))); };
		return *this;
	}
};

// Everything described about a type: its properties, and optionally a name to show for an instance of it (e.g. a
// sub-mesh's name). Built once per type by DescribeType<T>().
struct TypeDescription
{
	using DisplayNameGetter = std::function<std::string(const void* object)>;
	using IconGetter = std::function<PropertyTexture(const void* object)>;

	std::vector<PropertyDescriptor> Properties;
	DisplayNameGetter GetDisplayName;
	// A small image shown beside an instance's name (e.g. a material slot's diffuse texture); null for none
	IconGetter GetIcon;
};
