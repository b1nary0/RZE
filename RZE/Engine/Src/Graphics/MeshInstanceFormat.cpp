#include <Graphics/MeshInstanceFormat.h>

#include <Graphics/GraphicsLayouts.h>
#include <Graphics/MeshDraw.h>

#include <Utils/Math/Matrix4x4.h>

#include <cstddef>
#include <cstring>
#include <initializer_list>
#include <vector>

namespace
{
	using Rendering::EDataClassification;
	using Rendering::EDataFormat;
	using Rendering::ShaderInputLayout;

	// A float4 of an instance
	constexpr size_t k_registerSize = 4 * sizeof(float);

	// The Forward format's elements are the MeshInstanceLayout's bytes as they are: two matrices of four columns, then
	// the material's two registers
	static_assert(sizeof(Matrix4x4) == 4 * k_registerSize, "A Matrix4x4 isn't four float4 columns");
	static_assert(offsetof(GraphicsLayouts::ObjectBufferLayout, InvTransform) == sizeof(Matrix4x4),
		"ObjectBufferLayout's matrices aren't side by side");
	static_assert(offsetof(GraphicsLayouts::MeshInstanceLayout, Material) == 2 * sizeof(Matrix4x4),
		"MeshInstanceLayout's material values don't follow its two matrices");
	static_assert(offsetof(GraphicsLayouts::InstanceMaterialLayout, Opacity) == k_registerSize,
		"InstanceMaterialLayout's first register isn't its tint and shininess");
	static_assert(sizeof(GraphicsLayouts::MeshInstanceLayout) == 10 * k_registerSize, "MeshInstanceLayout isn't ten float4s");

	// A vertex shader's whole input: the mesh vertex in slot 0 (VertexInput in Common/ForwardTypes.hlsli), the same
	// for every format, then the instance's registers in slot 1, one a semantic, each 16 bytes after the last
	ShaderInputLayout MakeInputLayout(std::initializer_list<const char*> instanceSemantics)
	{
		std::vector<ShaderInputLayout::ShaderInputLayoutData> elements =
		{
			{ "POSITION", EDataFormat::R32G32B32_FLOAT, EDataClassification::PER_VERTEX, 0 },
			{ "NORMAL", EDataFormat::R32G32B32_FLOAT, EDataClassification::PER_VERTEX, 12 },
			{ "UV", EDataFormat::R32G32_FLOAT, EDataClassification::PER_VERTEX, 24 },
			{ "TANGENT", EDataFormat::R32G32B32_FLOAT, EDataClassification::PER_VERTEX, 32 }
		};

		size_t offset = 0;
		for (const char* semantic : instanceSemantics)
		{
			elements.emplace_back(semantic, EDataFormat::R32G32B32A32_FLOAT, EDataClassification::PER_INSTANCE, offset, MeshInstanceFormats::k_instanceSlot);
			offset += k_registerSize;
		}
		return ShaderInputLayout(std::move(elements));
	}

	void FillForward(const MeshDraw& draw, void* out)
	{
		const GraphicsLayouts::MeshInstanceLayout instance = draw.BuildInstance();
		std::memcpy(out, &instance, sizeof(instance));
	}
}

const MeshInstanceFormat& MeshInstanceFormats::Forward()
{
	// MeshInstanceInput's semantics, in its order
	static const MeshInstanceFormat format =
	{
		static_cast<U32>(sizeof(GraphicsLayouts::MeshInstanceLayout)),
		MakeInputLayout({ "WORLDA", "WORLDB", "WORLDC", "WORLDD", "INVWORLDA", "INVWORLDB", "INVWORLDC", "INVWORLDD", "MATERIALA", "MATERIALB" }),
		&FillForward
	};
	return format;
}
