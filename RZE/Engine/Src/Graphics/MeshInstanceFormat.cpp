#include <Graphics/MeshInstanceFormat.h>

#include <Graphics/GraphicsLayouts.h>
#include <Graphics/MeshDraw.h>

#include <Utils/DebugUtils/Debug.h>
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

	// The Depth format's elements are four float3s, side by side
	constexpr size_t k_columnSize = 3 * sizeof(float);
	static_assert(sizeof(GraphicsLayouts::MeshDepthInstanceLayout) == 4 * k_columnSize, "MeshDepthInstanceLayout isn't four float3s");

	// The mesh vertex's elements (VertexInput in Common/ForwardTypes.hlsli), where they are in a vertex
	const ShaderInputLayout::ShaderInputLayoutData k_vertexElements[] =
	{
		{ "POSITION", EDataFormat::R32G32B32_FLOAT, EDataClassification::PER_VERTEX, 0 },
		{ "NORMAL", EDataFormat::R32G32B32_FLOAT, EDataClassification::PER_VERTEX, 12 },
		{ "UV", EDataFormat::R32G32_FLOAT, EDataClassification::PER_VERTEX, 24 },
		{ "TANGENT", EDataFormat::R32G32B32_FLOAT, EDataClassification::PER_VERTEX, 32 }
	};

	// A vertex shader's whole input: the mesh vertex's elements it reads in slot 0, in k_vertexElements' order and at
	// their places in the vertex, then the instance's in slot 1, one a semantic, each elementSize bytes after the last
	ShaderInputLayout MakeInputLayout(std::initializer_list<const char*> vertexSemantics, EDataFormat instanceFormat, size_t elementSize,
		std::initializer_list<const char*> instanceSemantics)
	{
		std::vector<ShaderInputLayout::ShaderInputLayoutData> elements;
		for (const ShaderInputLayout::ShaderInputLayoutData& vertexElement : k_vertexElements)
		{
			for (const char* semantic : vertexSemantics)
			{
				if (vertexElement.Name == semantic)
				{
					elements.push_back(vertexElement);
				}
			}
		}
		AssertExpr(elements.size() == vertexSemantics.size());

		size_t offset = 0;
		for (const char* semantic : instanceSemantics)
		{
			elements.emplace_back(semantic, instanceFormat, EDataClassification::PER_INSTANCE, offset, MeshInstanceFormats::k_instanceSlot);
			offset += elementSize;
		}
		return ShaderInputLayout(std::move(elements));
	}

	void FillForward(const MeshDraw& draw, void* out)
	{
		const GraphicsLayouts::MeshInstanceLayout instance = draw.BuildInstance();
		std::memcpy(out, &instance, sizeof(instance));
	}

	void FillDepth(const MeshDraw& draw, void* out)
	{
		// Column by column, as glm stores them: each column's first three values
		const float* const transform = draw.Transform->GetValuePtr();
		GraphicsLayouts::MeshDepthInstanceLayout instance;
		for (size_t column = 0; column < 4; ++column)
		{
			std::memcpy(instance.WorldColumns[column], transform + 4 * column, k_columnSize);
		}
		std::memcpy(out, &instance, sizeof(instance));
	}
}

const MeshInstanceFormat& MeshInstanceFormats::Forward()
{
	// MeshInstanceInput's semantics, in its order
	static const MeshInstanceFormat format =
	{
		static_cast<U32>(sizeof(GraphicsLayouts::MeshInstanceLayout)),
		MakeInputLayout({ "POSITION", "NORMAL", "UV", "TANGENT" }, EDataFormat::R32G32B32A32_FLOAT, k_registerSize,
			{ "WORLDA", "WORLDB", "WORLDC", "WORLDD", "INVWORLDA", "INVWORLDB", "INVWORLDC", "INVWORLDD", "MATERIALA", "MATERIALB" }),
		&FillForward
	};
	return format;
}

const MeshInstanceFormat& MeshInstanceFormats::Depth()
{
	// DepthVertexInput's and MeshDepthInstanceInput's semantics, in their order
	static const MeshInstanceFormat format =
	{
		static_cast<U32>(sizeof(GraphicsLayouts::MeshDepthInstanceLayout)),
		MakeInputLayout({ "POSITION", "UV" }, EDataFormat::R32G32B32_FLOAT, k_columnSize, { "WORLDA", "WORLDB", "WORLDC", "WORLDD" }),
		&FillDepth
	};
	return format;
}
