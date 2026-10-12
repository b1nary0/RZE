#pragma once

#include <Rendering/BufferHandle.h>

#include <Utils/Math/Vector2D.h>
#include <Utils/Math/Vector3D.h>
#include <Utils/Math/Vector4D.h>
#include <Utils/Platform/FilePath.h>

#include <functional>
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
	// Shown instead of the type's display name, e.g. a material's texture slot ("Diffuse")
	std::string Label;

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
// type. The alternatives' order matches EPropertyType.
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
	// A list of described objects, each shown nested; read-only
	Array,
	// An image; read-only
	Texture
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

	std::string Name;
	EPropertyType Type = EPropertyType::Float;
	Getter Get;
	// Null for a read-only property
	Setter Set;

	// Inclusive limits for numeric types; ignored when Min >= Max
	float Min = 0.0f;
	float Max = 0.0f;
	// Per step of an input's +/- buttons, or per pixel of a drag; 0 lets the editor choose
	float Step = 0.0f;
	// printf format for floats, e.g. "%.2f"; empty lets the editor choose
	std::string Format;
	std::string Tooltip;
	EPropertyWidget Widget = EPropertyWidget::Default;

	// For AssetPath properties: the file prompt's title, the folder it starts in, and its filter (e.g. "*.meshasset")
	std::string AssetDescription;
	std::string AssetDirectory;
	std::string AssetFilter;

	bool IsReadOnly() const { return !Set; }
	bool HasRange() const { return Min < Max; }

	PropertyDescriptor& Range(float min, float max) { Min = min; Max = max; return *this; }
	PropertyDescriptor& StepSize(float step) { Step = step; return *this; }
	PropertyDescriptor& DisplayFormat(const char* format) { Format = format; return *this; }
	PropertyDescriptor& Tip(const char* tooltip) { Tooltip = tooltip; return *this; }
	PropertyDescriptor& As(EPropertyWidget widget) { Widget = widget; return *this; }
	PropertyDescriptor& Asset(const char* description, const char* directory, const char* filter)
	{
		AssetDescription = description;
		AssetDirectory = directory;
		AssetFilter = filter;
		return *this;
	}
};

// Everything described about a type: its properties, and optionally a name to show for an instance of it (e.g. a
// sub-mesh's name). Built once per type by DescribeType<T>().
struct TypeDescription
{
	using DisplayNameGetter = std::function<std::string(const void* object)>;

	std::vector<PropertyDescriptor> Properties;
	DisplayNameGetter GetDisplayName;
};
