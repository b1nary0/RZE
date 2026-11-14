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

	// The ID format's are the Depth format's, then the ID's two halves, a uint each
	constexpr size_t k_idHalfSize = sizeof(U32);
	static_assert(offsetof(GraphicsLayouts::MeshIDInstanceLayout, ID) == sizeof(GraphicsLayouts::MeshDepthInstanceLayout),
		"MeshIDInstanceLayout's ID doesn't follow its transform");
	static_assert(sizeof(GraphicsLayouts::MeshIDInstanceLayout) == 4 * k_columnSize + 2 * k_idHalfSize,
		"MeshIDInstanceLayout isn't four float3s and two uints");

	// The mesh vertex's elements (VertexInput in Common/ForwardTypes.hlsli), where they are in a vertex
	const ShaderInputLayout::ShaderInputLayoutData k_vertexElements[] =
	{
		{ "POSITION", EDataFormat::R32G32B32_FLOAT, EDataClassification::PER_VERTEX, 0 },
		{ "NORMAL", EDataFormat::R32G32B32_FLOAT, EDataClassification::PER_VERTEX, 12 },
		{ "UV", EDataFormat::R32G32_FLOAT, EDataClassification::PER_VERTEX, 24 },
		{ "TANGENT", EDataFormat::R32G32B32_FLOAT, EDataClassification::PER_VERTEX, 32 }
	};

	// Instance elements of one format, side by side: one a semantic, each Size bytes
	struct InstanceElements
	{
		EDataFormat Format;
		size_t Size;
		std::initializer_list<const char*> Semantics;
	};

	// A vertex shader's whole input: the mesh vertex's elements it reads in slot 0, in k_vertexElements' order and at
	// their places in the vertex, then the instance's in slot 1, each run's after the last's
	ShaderInputLayout MakeInputLayout(std::initializer_list<const char*> vertexSemantics, std::initializer_list<InstanceElements> instanceRuns)
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
		for (const InstanceElements& run : instanceRuns)
		{
			for (const char* semantic : run.Semantics)
			{
				elements.emplace_back(semantic, run.Format, EDataClassification::PER_INSTANCE, offset, MeshInstanceFormats::k_instanceSlot);
				offset += run.Size;
			}
		}
		return ShaderInputLayout(std::move(elements));
	}

	// The world transform as the depth passes read it: column by column, as glm stores them, each column's first three
	// values
	GraphicsLayouts::MeshDepthInstanceLayout BuildDepthInstance(const MeshDraw& draw)
	{
		const float* const transform = draw.Transform->GetValuePtr();
		GraphicsLayouts::MeshDepthInstanceLayout instance;
		for (size_t column = 0; column < 4; ++column)
		{
			std::memcpy(instance.WorldColumns[column], transform + 4 * column, k_columnSize);
		}
		return instance;
	}

	void FillForward(const MeshDraw& draw, U64 /*id*/, void* out)
	{
		const GraphicsLayouts::MeshInstanceLayout instance = draw.BuildInstance();
		std::memcpy(out, &instance, sizeof(instance));
	}

	void FillDepth(const MeshDraw& draw, U64 /*id*/, void* out)
	{
		const GraphicsLayouts::MeshDepthInstanceLayout instance = BuildDepthInstance(draw);
		std::memcpy(out, &instance, sizeof(instance));
	}

	void FillID(const MeshDraw& draw, U64 id, void* out)
	{
		GraphicsLayouts::MeshIDInstanceLayout instance;
		instance.World = BuildDepthInstance(draw);
		instance.ID[0] = static_cast<U32>(id);
		instance.ID[1] = static_cast<U32>(id >> 32);
		std::memcpy(out, &instance, sizeof(instance));
	}
}

const MeshInstanceFormat& MeshInstanceFormats::Forward()
{
	// MeshInstanceInput's semantics, in its order
	static const MeshInstanceFormat format =
	{
		static_cast<U32>(sizeof(GraphicsLayouts::MeshInstanceLayout)),
		MakeInputLayout({ "POSITION", "NORMAL", "UV", "TANGENT" }, { { EDataFormat::R32G32B32A32_FLOAT, k_registerSize,
			{ "WORLDA", "WORLDB", "WORLDC", "WORLDD", "INVWORLDA", "INVWORLDB", "INVWORLDC", "INVWORLDD", "MATERIALA", "MATERIALB" } } }),
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
		MakeInputLayout({ "POSITION", "UV" }, { { EDataFormat::R32G32B32_FLOAT, k_columnSize, { "WORLDA", "WORLDB", "WORLDC", "WORLDD" } } }),
		&FillDepth
	};
	return format;
}

const MeshInstanceFormat& MeshInstanceFormats::ID()
{
	// DepthVertexInput's and MeshIDInstanceInput's semantics, in their order
	static const MeshInstanceFormat format =
	{
		static_cast<U32>(sizeof(GraphicsLayouts::MeshIDInstanceLayout)),
		MakeInputLayout({ "POSITION", "UV" }, {
			{ EDataFormat::R32G32B32_FLOAT, k_columnSize, { "WORLDA", "WORLDB", "WORLDC", "WORLDD" } },
			{ EDataFormat::R32_UINT, k_idHalfSize, { "IDLOW", "IDHIGH" } } }),
		&FillID,
		true
	};
	return format;
}
