#include <doctest/doctest.h>

#include <Graphics/GraphicsLayouts.h>
#include <Graphics/MaterialOverrides.h>
#include <Graphics/MaterialSlot.h>
#include <Graphics/MeshBatchGrouping.h>
#include <Graphics/MeshDraw.h>
#include <Graphics/MeshGeometry.h>
#include <Graphics/MeshInstanceFormat.h>
#include <Graphics/MeshInstanceStream.h>

#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector3D.h>

#include <cstring>
#include <vector>

namespace
{
	// A slot without a material (making one needs the renderer): its draws carry the default values, with overrides
	// on top
	MaterialSlot MakeSlot(const MaterialOverrides& overrides)
	{
		MaterialSlot slot{ StaticMesh::Slot() };
		slot.SetOverrides(overrides);
		return slot;
	}

	// Draws of geometry, each placed somewhere of its own with its own matrices, as a sub-mesh away from its mesh's
	// origin is, all of one slot nothing overrides
	std::vector<MeshDraw> MakeDraws(size_t count, const MeshGeometry& geometry)
	{
		static const MaterialSlot slot = MakeSlot(MaterialOverrides());

		std::vector<MeshDraw> draws(count);
		for (size_t index = 0; index < count; ++index)
		{
			MeshDraw& draw = draws[index];
			const float offset = static_cast<float>(index + 1);
			draw.Geometry = &geometry;
			draw.Slot = &slot;
			draw.OwnMatrices.Transform = Matrix4x4::CreateInPlace(Vector3D(offset, 2.0f * offset, -offset), Vector3D(1.0f), Vector3D(0.0f, 10.0f * offset, 0.0f));
			draw.OwnMatrices.InvTransform = draw.OwnMatrices.Transform.Inverse();
			draw.Transform = &draw.OwnMatrices.Transform;
		}
		return draws;
	}

	// Stand-ins for what a stage binds: only their addresses are compared
	const int k_materialA = 0;
	const int k_materialB = 0;

	// The batches cover the instances in order, with no gaps
	void CheckContiguous(const MeshBatchGrouping& grouping)
	{
		U32 next = 0;
		for (const MeshBatchGrouping::Batch& batch : grouping.GetBatches())
		{
			CHECK(batch.FirstInstance == next);
			CHECK(batch.InstanceCount > 0);
			next += batch.InstanceCount;
		}
		CHECK(next == grouping.GetInstanceCount());
	}

	// Each instance's draw, in the instances' order: every instance placed once
	std::vector<const MeshDraw*> InstanceDraws(const MeshBatchGrouping& grouping)
	{
		std::vector<const MeshDraw*> instanceDraws(grouping.GetInstanceCount(), nullptr);
		for (const MeshBatchGrouping::Placement& placement : grouping.GetPlacements())
		{
			REQUIRE(placement.Instance < instanceDraws.size());
			CHECK(instanceDraws[placement.Instance] == nullptr);
			instanceDraws[placement.Instance] = placement.Draw;
		}
		return instanceDraws;
	}

	// Builds a stream as MeshBatches does, for a format without an ID
	void BuildStream(MeshInstanceStream& stream, const MeshBatchGrouping& grouping)
	{
		stream.Build(grouping.GetPlacements(), {});
	}

	// An ID stream's instance, as the GPU would read it
	GraphicsLayouts::MeshIDInstanceLayout ReadIDInstance(const MeshInstanceStream& stream, U32 index)
	{
		GraphicsLayouts::MeshIDInstanceLayout instance;
		std::memcpy(&instance, stream.GetInstance(index), sizeof(instance));
		return instance;
	}

	// A Forward stream's instance, as the GPU would read it
	GraphicsLayouts::MeshInstanceLayout ReadInstance(const MeshInstanceStream& stream, U32 index)
	{
		GraphicsLayouts::MeshInstanceLayout instance;
		std::memcpy(&instance, stream.GetInstance(index), sizeof(instance));
		return instance;
	}
}

TEST_CASE("Draws with the same bind key and geometry are one batch; either differing splits them, keys together")
{
	const MeshGeometry geometryA;
	const MeshGeometry geometryB;
	const std::vector<MeshDraw> drawsA = MakeDraws(3, geometryA);
	const std::vector<MeshDraw> drawsB = MakeDraws(2, geometryB);

	MeshBatchGrouping grouping;
	grouping.Add(drawsA[0], &k_materialA, nullptr);
	grouping.Add(drawsA[1], &k_materialA, nullptr);
	// Same geometry, another key
	grouping.Add(drawsA[2], &k_materialB, nullptr);
	// Same key, another geometry
	grouping.Add(drawsB[0], &k_materialA, nullptr);
	grouping.Add(drawsB[1], &k_materialA, nullptr);
	grouping.Build();

	// Key A's two batches come together, ahead of key B's: A's first draw was added first
	const std::vector<MeshBatchGrouping::Batch>& built = grouping.GetBatches();
	REQUIRE(built.size() == 3);
	CHECK(built[0].Draw == &drawsA[0]);
	CHECK(built[0].InstanceCount == 2);
	CHECK(built[1].Draw == &drawsB[0]);
	CHECK(built[1].InstanceCount == 2);
	CHECK(built[2].Draw == &drawsA[2]);
	CHECK(built[2].InstanceCount == 1);
	CheckContiguous(grouping);
}

TEST_CASE("Grouped batches sort by order key, then bind key, each ranked by its first draw, ties in the order added")
{
	const MeshGeometry geometries[4];
	std::vector<MeshDraw> draws = MakeDraws(4, geometries[0]);
	for (size_t index = 0; index < draws.size(); ++index)
	{
		draws[index].Geometry = &geometries[index];
	}

	// Stand-ins for two pixel shaders and a third material: only their addresses are compared
	const int shaderX = 0;
	const int shaderY = 0;
	const int materialC = 0;

	MeshBatchGrouping grouping;
	// Shader Y is seen first, so its batches come first; among them, material B's before C's
	grouping.Add(draws[0], &k_materialB, &shaderY);
	grouping.Add(draws[1], &k_materialA, &shaderX);
	grouping.Add(draws[2], &materialC, &shaderY);
	grouping.Add(draws[3], &k_materialB, &shaderY);
	grouping.Build();

	const std::vector<MeshBatchGrouping::Batch>& built = grouping.GetBatches();
	REQUIRE(built.size() == 4);
	CHECK(built[0].Draw == &draws[0]);
	CHECK(built[1].Draw == &draws[3]);
	CHECK(built[2].Draw == &draws[2]);
	CHECK(built[3].Draw == &draws[1]);
	CheckContiguous(grouping);

	// The instances follow their batches
	const std::vector<const MeshDraw*> instanceDraws = InstanceDraws(grouping);
	REQUIRE(instanceDraws.size() == 4);
	CHECK(instanceDraws[0] == &draws[0]);
	CHECK(instanceDraws[1] == &draws[3]);
	CHECK(instanceDraws[2] == &draws[2]);
	CHECK(instanceDraws[3] == &draws[1]);
}

TEST_CASE("Grouped batches sort only among their neighbours: in-order batches stay where they were added")
{
	const MeshGeometry geometries[5];
	std::vector<MeshDraw> draws = MakeDraws(5, geometries[0]);
	for (size_t index = 0; index < draws.size(); ++index)
	{
		draws[index].Geometry = &geometries[index];
	}

	const int shaderX = 0;
	const int shaderY = 0;

	MeshBatchGrouping grouping;
	grouping.Add(draws[0], &k_materialB, &shaderY);
	grouping.Add(draws[1], &k_materialA, &shaderX);
	grouping.AddInOrder(draws[2], &k_materialA);
	// After the in-order draw: these two sort between themselves, Y (ranked first) ahead of X
	grouping.Add(draws[3], &k_materialA, &shaderX);
	grouping.Add(draws[4], &k_materialB, &shaderY);
	grouping.Build();

	const std::vector<MeshBatchGrouping::Batch>& built = grouping.GetBatches();
	REQUIRE(built.size() == 5);
	CHECK(built[0].Draw == &draws[0]);
	CHECK(built[1].Draw == &draws[1]);
	CHECK(built[2].Draw == &draws[2]);
	CHECK(built[3].Draw == &draws[4]);
	CHECK(built[4].Draw == &draws[3]);
	CheckContiguous(grouping);
}

TEST_CASE("Batches come where their first draw was added, and a batch's instances in the order added")
{
	const MeshGeometry geometryA;
	const MeshGeometry geometryB;
	// 0 to 2 of geometry A, 3 and 4 of B
	std::vector<MeshDraw> draws = MakeDraws(5, geometryA);
	draws[3].Geometry = &geometryB;
	draws[4].Geometry = &geometryB;

	// Interleaved, as a scene's objects come
	MeshBatchGrouping grouping;
	grouping.Add(draws[3], nullptr, nullptr);
	grouping.Add(draws[2], nullptr, nullptr);
	grouping.Add(draws[4], nullptr, nullptr);
	grouping.Add(draws[0], nullptr, nullptr);
	grouping.Add(draws[1], nullptr, nullptr);
	grouping.Build();

	const std::vector<MeshBatchGrouping::Batch>& built = grouping.GetBatches();
	REQUIRE(built.size() == 2);
	CHECK(built[0].Draw == &draws[3]);
	CHECK(built[0].InstanceCount == 2);
	CHECK(built[1].Draw == &draws[2]);
	CHECK(built[1].InstanceCount == 3);
	CheckContiguous(grouping);

	const std::vector<const MeshDraw*> instanceDraws = InstanceDraws(grouping);
	REQUIRE(instanceDraws.size() == 5);
	CHECK(instanceDraws[0] == &draws[3]);
	CHECK(instanceDraws[1] == &draws[4]);
	CHECK(instanceDraws[2] == &draws[2]);
	CHECK(instanceDraws[3] == &draws[0]);
	CHECK(instanceDraws[4] == &draws[1]);
}

TEST_CASE("In-order draws come in the order added, and neighbours with the same key and geometry share a batch")
{
	const MeshGeometry geometry;
	const std::vector<MeshDraw> draws = MakeDraws(5, geometry);

	MeshBatchGrouping grouping;
	grouping.Add(draws[0], &k_materialA, nullptr);
	// The same key and geometry as the grouped draw, but in order: apart from it
	grouping.AddInOrder(draws[3], &k_materialA);
	grouping.AddInOrder(draws[1], &k_materialA);
	grouping.AddInOrder(draws[4], &k_materialB);
	grouping.AddInOrder(draws[2], &k_materialB);
	grouping.Build();

	const std::vector<MeshBatchGrouping::Batch>& built = grouping.GetBatches();
	REQUIRE(built.size() == 3);
	CHECK(built[0].Draw == &draws[0]);
	CHECK(built[0].InstanceCount == 1);
	CHECK(built[1].Draw == &draws[3]);
	CHECK(built[1].InstanceCount == 2);
	CHECK(built[2].Draw == &draws[4]);
	CHECK(built[2].InstanceCount == 2);
	CheckContiguous(grouping);

	// Each run's instances in the order added, so a sorted order holds within the batch
	const std::vector<const MeshDraw*> instanceDraws = InstanceDraws(grouping);
	REQUIRE(instanceDraws.size() == 5);
	CHECK(instanceDraws[1] == &draws[3]);
	CHECK(instanceDraws[2] == &draws[1]);
	CHECK(instanceDraws[3] == &draws[4]);
	CHECK(instanceDraws[4] == &draws[2]);
}

TEST_CASE("In-order draws of another geometry, or with another key between, don't share a batch")
{
	const MeshGeometry geometry;
	const MeshGeometry otherGeometry;
	std::vector<MeshDraw> draws = MakeDraws(5, geometry);
	draws[1].Geometry = &otherGeometry;

	MeshBatchGrouping grouping;
	grouping.AddInOrder(draws[0], &k_materialA);
	// Another geometry
	grouping.AddInOrder(draws[1], &k_materialA);
	grouping.AddInOrder(draws[2], &k_materialA);
	// A B A: joining the first A would move it ahead of B
	grouping.AddInOrder(draws[3], &k_materialB);
	grouping.AddInOrder(draws[4], &k_materialA);
	grouping.Build();

	const std::vector<MeshBatchGrouping::Batch>& built = grouping.GetBatches();
	REQUIRE(built.size() == 5);
	for (size_t index = 0; index < built.size(); ++index)
	{
		CHECK(built[index].Draw == &draws[index]);
		CHECK(built[index].InstanceCount == 1);
	}
	CheckContiguous(grouping);
}

TEST_CASE("A grouped draw added between two in-order draws keeps them apart")
{
	const MeshGeometry geometry;
	const std::vector<MeshDraw> draws = MakeDraws(3, geometry);

	MeshBatchGrouping grouping;
	grouping.AddInOrder(draws[0], &k_materialA);
	grouping.Add(draws[1], &k_materialB, nullptr);
	grouping.AddInOrder(draws[2], &k_materialA);
	grouping.Build();

	const std::vector<MeshBatchGrouping::Batch>& built = grouping.GetBatches();
	REQUIRE(built.size() == 3);
	CHECK(built[0].Draw == &draws[0]);
	CHECK(built[1].Draw == &draws[1]);
	CHECK(built[2].Draw == &draws[2]);
	CheckContiguous(grouping);
}

TEST_CASE("A grouped draw added after an in-order one joins its batch ahead of it")
{
	const MeshGeometry geometry;
	const std::vector<MeshDraw> draws = MakeDraws(3, geometry);

	MeshBatchGrouping grouping;
	grouping.Add(draws[0], nullptr, nullptr);
	grouping.AddInOrder(draws[1], nullptr);
	grouping.Add(draws[2], nullptr, nullptr);
	grouping.Build();

	const std::vector<MeshBatchGrouping::Batch>& built = grouping.GetBatches();
	REQUIRE(built.size() == 2);
	CHECK(built[0].Draw == &draws[0]);
	CHECK(built[0].InstanceCount == 2);
	CHECK(built[1].Draw == &draws[1]);
	CheckContiguous(grouping);
}

TEST_CASE("Each instance is its draw's transform and inverse, shared matrices included")
{
	const MeshGeometry geometry;
	// As a sub-mesh at its mesh's origin: Transform points at the object's ObjectBufferLayout, not the draw's own
	GraphicsLayouts::ObjectBufferLayout objectMatrices;
	objectMatrices.Transform = Matrix4x4::CreateInPlace(Vector3D(5.0f, 0.0f, 0.0f), Vector3D(2.0f), Vector3D(0.0f));
	objectMatrices.InvTransform = objectMatrices.Transform.Inverse();

	std::vector<MeshDraw> draws = MakeDraws(2, geometry);
	draws[1].Transform = &objectMatrices.Transform;

	MeshBatchGrouping grouping;
	grouping.Add(draws[0], nullptr, nullptr);
	grouping.Add(draws[1], nullptr, nullptr);
	grouping.Build();

	MeshInstanceStream stream(MeshInstanceFormats::Forward());
	BuildStream(stream, grouping);
	REQUIRE(stream.GetCount() == 2);
	const GraphicsLayouts::MeshInstanceLayout first = ReadInstance(stream, 0);
	const GraphicsLayouts::MeshInstanceLayout second = ReadInstance(stream, 1);
	CHECK(first.Object.Transform == draws[0].OwnMatrices.Transform);
	CHECK(first.Object.InvTransform == draws[0].OwnMatrices.InvTransform);
	CHECK(second.Object.Transform == objectMatrices.Transform);
	CHECK(second.Object.InvTransform == objectMatrices.InvTransform);
}

TEST_CASE("Draws of slots that differ only in tint, shininess or opacity share a batch, each instance with its own")
{
	const MeshGeometry geometry;

	MaterialOverrides red;
	red.Tint = Vector3D(1.0f, 0.0f, 0.0f);
	MaterialOverrides shiny;
	shiny.Shininess = 64.0f;
	const MaterialSlot plainSlot = MakeSlot(MaterialOverrides());
	const MaterialSlot redSlot = MakeSlot(red);
	const MaterialSlot shinySlot = MakeSlot(shiny);

	std::vector<MeshDraw> draws = MakeDraws(3, geometry);
	draws[0].Slot = &plainSlot;
	draws[1].Slot = &redSlot;
	draws[2].Slot = &shinySlot;

	// The same bind key: none of the three overrides anything a draw binds, so a stage is binding one material
	MeshBatchGrouping grouping;
	for (const MeshDraw& draw : draws)
	{
		grouping.Add(draw, &k_materialA, nullptr);
	}
	grouping.Build();

	REQUIRE(grouping.GetBatches().size() == 1);
	CHECK(grouping.GetBatches()[0].InstanceCount == 3);

	MeshInstanceStream stream(MeshInstanceFormats::Forward());
	BuildStream(stream, grouping);
	REQUIRE(stream.GetCount() == 3);
	std::vector<GraphicsLayouts::MeshInstanceLayout> instances;
	for (U32 index = 0; index < stream.GetCount(); ++index)
	{
		instances.push_back(ReadInstance(stream, index));
	}
	const GraphicsLayouts::InstanceMaterialLayout defaults;
	CHECK(instances[0].Material.Tint[1] == doctest::Approx(1.0f));
	CHECK(instances[0].Material.Shininess == doctest::Approx(defaults.Shininess));
	CHECK(instances[1].Material.Tint[0] == doctest::Approx(1.0f));
	CHECK(instances[1].Material.Tint[1] == doctest::Approx(0.0f));
	CHECK(instances[1].Material.Tint[2] == doctest::Approx(0.0f));
	CHECK(instances[2].Material.Tint[1] == doctest::Approx(1.0f));
	CHECK(instances[2].Material.Shininess == doctest::Approx(64.0f));
	for (const GraphicsLayouts::MeshInstanceLayout& instance : instances)
	{
		CHECK(instance.Material.Opacity == doctest::Approx(1.0f));
	}
}

TEST_CASE("Building again after Clear replaces the last pass's batches, and a stream's instances with the new draws'")
{
	const MeshGeometry geometry;
	const std::vector<MeshDraw> draws = MakeDraws(4, geometry);

	MeshBatchGrouping grouping;
	// Each its own key, so each its own batch
	for (const MeshDraw& draw : draws)
	{
		grouping.AddInOrder(draw, &draw);
	}
	grouping.Build();
	REQUIRE(grouping.GetBatches().size() == 4);

	MeshInstanceStream stream(MeshInstanceFormats::Forward());
	BuildStream(stream, grouping);
	REQUIRE(stream.GetCount() == 4);

	grouping.Clear();
	grouping.Add(draws[3], nullptr, nullptr);
	grouping.Build();
	REQUIRE(grouping.GetBatches().size() == 1);
	REQUIRE(grouping.GetInstanceCount() == 1);
	CHECK(grouping.GetBatches()[0].Draw == &draws[3]);
	CHECK(grouping.GetBatches()[0].FirstInstance == 0);
	BuildStream(stream, grouping);
	REQUIRE(stream.GetCount() == 1);
	CHECK(ReadInstance(stream, 0).Object.Transform == draws[3].OwnMatrices.Transform);

	grouping.Clear();
	grouping.Build();
	CHECK(grouping.GetBatches().empty());
	CHECK(grouping.GetInstanceCount() == 0);
	BuildStream(stream, grouping);
	CHECK(stream.GetCount() == 0);
}

TEST_CASE("A stream's instances are its draws', each where it's placed, as the format fills them")
{
	const MeshGeometry geometry;
	const std::vector<MeshDraw> draws = MakeDraws(3, geometry);
	// Placed out of order, one draw twice
	const std::vector<MeshBatchGrouping::Placement> placements = { { &draws[0], 1 }, { &draws[1], 2 }, { &draws[2], 0 }, { &draws[0], 3 } };
	const std::vector<const MeshDraw*> instanceDraws = { &draws[2], &draws[0], &draws[1], &draws[0] };

	MeshInstanceStream stream(MeshInstanceFormats::Forward());
	stream.Build(placements, {});
	REQUIRE(stream.GetCount() == instanceDraws.size());
	for (U32 index = 0; index < stream.GetCount(); ++index)
	{
		CAPTURE(index);
		const GraphicsLayouts::MeshInstanceLayout expected = instanceDraws[index]->BuildInstance();
		CHECK(std::memcmp(stream.GetInstance(index), &expected, sizeof(expected)) == 0);
	}
}

TEST_CASE("The Forward format reads the vertex from slot 0 and the instance's 10 registers from slot 1, 16 bytes apart")
{
	const MeshInstanceFormat& format = MeshInstanceFormats::Forward();
	CHECK(format.Stride == sizeof(GraphicsLayouts::MeshInstanceLayout));
	const Rendering::ShaderInputLayout& layout = format.InputLayout;
	REQUIRE(layout.GetDataCount() == 14);

	for (size_t index = 0; index < 4; ++index)
	{
		CAPTURE(index);
		CHECK(layout[index].InputSlot == 0);
		CHECK(layout[index].DataClassification == Rendering::EDataClassification::PER_VERTEX);
	}

	// Two matrices' columns, then the material's two registers
	for (size_t column = 0; column < 10; ++column)
	{
		const Rendering::ShaderInputLayout::ShaderInputLayoutData& element = layout[4 + column];
		CAPTURE(element.Name);
		CHECK(element.InputSlot == MeshInstanceFormats::k_instanceSlot);
		CHECK(element.DataClassification == Rendering::EDataClassification::PER_INSTANCE);
		CHECK(element.DataFormat == Rendering::EDataFormat::R32G32B32A32_FLOAT);
		CHECK(element.Offset == column * 16);
		// D3D11 takes semantic index 0 for every element, so a trailing digit wouldn't match the shader's semantic
		CHECK(!element.Name.empty());
		CHECK((element.Name.back() < '0' || element.Name.back() > '9'));
	}
	CHECK(layout[13].Offset + 16 == sizeof(GraphicsLayouts::MeshInstanceLayout));
}

TEST_CASE("The Depth format reads the vertex's position and UV where they are, and three float3 columns from slot 1")
{
	const MeshInstanceFormat& format = MeshInstanceFormats::Depth();
	CHECK(format.Stride == sizeof(GraphicsLayouts::MeshDepthInstanceLayout));
	CHECK(format.Stride == 48);

	const Rendering::ShaderInputLayout& layout = format.InputLayout;
	REQUIRE(layout.GetDataCount() == 6);

	// The same places in the vertex as the Forward format reads them, as both read the same vertex buffer
	const Rendering::ShaderInputLayout& forward = MeshInstanceFormats::Forward().InputLayout;
	CHECK(layout[0].Name == "POSITION");
	CHECK(layout[0].Offset == forward[0].Offset);
	CHECK(layout[1].Name == "UV");
	CHECK(layout[1].Offset == forward[2].Offset);
	for (size_t index = 0; index < 2; ++index)
	{
		CAPTURE(index);
		CHECK(layout[index].InputSlot == 0);
		CHECK(layout[index].DataClassification == Rendering::EDataClassification::PER_VERTEX);
	}

	for (size_t column = 0; column < 4; ++column)
	{
		const Rendering::ShaderInputLayout::ShaderInputLayoutData& element = layout[2 + column];
		CAPTURE(element.Name);
		CHECK(element.InputSlot == MeshInstanceFormats::k_instanceSlot);
		CHECK(element.DataClassification == Rendering::EDataClassification::PER_INSTANCE);
		CHECK(element.DataFormat == Rendering::EDataFormat::R32G32B32_FLOAT);
		CHECK(element.Offset == column * 12);
		CHECK(!element.Name.empty());
		CHECK((element.Name.back() < '0' || element.Name.back() > '9'));
	}
}

TEST_CASE("The DepthPosition format reads only the vertex's position, and the Depth format's instances from slot 1")
{
	const MeshInstanceFormat& format = MeshInstanceFormats::DepthPosition();
	const MeshInstanceFormat& depth = MeshInstanceFormats::Depth();
	// The same instances, so a stream of Depth's feeds a shader reading either
	CHECK(format.Stride == depth.Stride);
	CHECK(format.Fill == depth.Fill);
	CHECK(format.CarriesID == depth.CarriesID);

	const Rendering::ShaderInputLayout& layout = format.InputLayout;
	REQUIRE(layout.GetDataCount() == 5);
	CHECK(layout[0].Name == "POSITION");
	CHECK(layout[0].Offset == 0);
	CHECK(layout[0].InputSlot == 0);
	CHECK(layout[0].DataFormat == Rendering::EDataFormat::R32G32B32_FLOAT);
	CHECK(layout[0].DataClassification == Rendering::EDataClassification::PER_VERTEX);

	// The instance's elements are Depth's, after its UV
	for (size_t column = 0; column < 4; ++column)
	{
		const Rendering::ShaderInputLayout::ShaderInputLayoutData& element = layout[1 + column];
		const Rendering::ShaderInputLayout::ShaderInputLayoutData& depthElement = depth.InputLayout[2 + column];
		CAPTURE(element.Name);
		CHECK(element.Name == depthElement.Name);
		CHECK(element.Offset == depthElement.Offset);
		CHECK(element.InputSlot == MeshInstanceFormats::k_instanceSlot);
		CHECK(element.DataClassification == Rendering::EDataClassification::PER_INSTANCE);
		CHECK(element.DataFormat == depthElement.DataFormat);
	}
}

TEST_CASE("A Depth instance is its draw's transform's columns without their fourth values, shared matrices included")
{
	const MeshGeometry geometry;
	GraphicsLayouts::ObjectBufferLayout objectMatrices;
	objectMatrices.Transform = Matrix4x4::CreateInPlace(Vector3D(5.0f, -3.0f, 7.0f), Vector3D(2.0f, 1.0f, 0.5f), Vector3D(30.0f, 45.0f, 60.0f));
	objectMatrices.InvTransform = objectMatrices.Transform.Inverse();

	std::vector<MeshDraw> draws = MakeDraws(2, geometry);
	draws[1].Transform = &objectMatrices.Transform;
	// No slot: a depth instance is built without it
	draws[0].Slot = nullptr;
	draws[1].Slot = nullptr;

	MeshBatchGrouping grouping;
	grouping.Add(draws[0], nullptr, nullptr);
	grouping.Add(draws[1], nullptr, nullptr);
	grouping.Build();

	MeshInstanceStream stream(MeshInstanceFormats::Depth());
	BuildStream(stream, grouping);
	REQUIRE(stream.GetCount() == 2);
	const Matrix4x4* const transforms[] = { &draws[0].OwnMatrices.Transform, &objectMatrices.Transform };
	for (U32 index = 0; index < 2; ++index)
	{
		CAPTURE(index);
		GraphicsLayouts::MeshDepthInstanceLayout instance;
		std::memcpy(&instance, stream.GetInstance(index), sizeof(instance));
		const float* const expected = transforms[index]->GetValuePtr();
		for (size_t column = 0; column < 4; ++column)
		{
			CAPTURE(column);
			// The columns' fourth values are the bottom row: an affine transform's 0, 0, 0, 1
			CHECK(expected[4 * column + 3] == (column == 3 ? 1.0f : 0.0f));
			for (size_t row = 0; row < 3; ++row)
			{
				CHECK(instance.WorldColumns[column][row] == expected[4 * column + row]);
			}
		}
	}
}

TEST_CASE("The ID format reads the Depth format's vertex and columns, then the ID's two halves as uints from slot 1")
{
	const MeshInstanceFormat& format = MeshInstanceFormats::ID();
	CHECK(format.CarriesID);
	CHECK(!MeshInstanceFormats::Forward().CarriesID);
	CHECK(!MeshInstanceFormats::Depth().CarriesID);
	CHECK(format.Stride == sizeof(GraphicsLayouts::MeshIDInstanceLayout));
	CHECK(format.Stride == 56);

	// The Depth format's elements, as they are there: both read Vertex_MeshDepthOnly's way
	const Rendering::ShaderInputLayout& layout = format.InputLayout;
	const Rendering::ShaderInputLayout& depth = MeshInstanceFormats::Depth().InputLayout;
	REQUIRE(layout.GetDataCount() == depth.GetDataCount() + 2);
	for (size_t index = 0; index < depth.GetDataCount(); ++index)
	{
		CAPTURE(index);
		CHECK(layout[index].Name == depth[index].Name);
		CHECK(layout[index].DataFormat == depth[index].DataFormat);
		CHECK(layout[index].Offset == depth[index].Offset);
		CHECK(layout[index].InputSlot == depth[index].InputSlot);
		CHECK(layout[index].DataClassification == depth[index].DataClassification);
	}

	const char* const halves[] = { "IDLOW", "IDHIGH" };
	for (size_t half = 0; half < 2; ++half)
	{
		const Rendering::ShaderInputLayout::ShaderInputLayoutData& element = layout[depth.GetDataCount() + half];
		CAPTURE(half);
		CHECK(element.Name == halves[half]);
		CHECK(element.InputSlot == MeshInstanceFormats::k_instanceSlot);
		CHECK(element.DataClassification == Rendering::EDataClassification::PER_INSTANCE);
		CHECK(element.DataFormat == Rendering::EDataFormat::R32_UINT);
		CHECK(element.Offset == sizeof(GraphicsLayouts::MeshDepthInstanceLayout) + half * sizeof(U32));
	}
}

TEST_CASE("An ID instance is its draw's Depth instance and the ID it was given, low half first")
{
	const MeshGeometry geometry;
	std::vector<MeshDraw> draws = MakeDraws(1, geometry);
	// No slot: an ID instance is built without it
	draws[0].Slot = nullptr;

	MeshBatchGrouping grouping;
	grouping.Add(draws[0], nullptr, nullptr);
	grouping.Build();

	MeshInstanceStream depthStream(MeshInstanceFormats::Depth());
	BuildStream(depthStream, grouping);
	MeshInstanceStream idStream(MeshInstanceFormats::ID());
	const U64 id = 0x89ABCDEF01234567ull;
	idStream.Build(grouping.GetPlacements(), { id });
	REQUIRE(idStream.GetCount() == 1);

	const GraphicsLayouts::MeshIDInstanceLayout instance = ReadIDInstance(idStream, 0);
	CHECK(std::memcmp(&instance.World, depthStream.GetInstance(0), sizeof(instance.World)) == 0);
	CHECK(instance.ID[0] == 0x01234567u);
	CHECK(instance.ID[1] == 0x89ABCDEFu);
}

TEST_CASE("Each draw's ID goes with it when grouping moves its instance ahead of draws added before it")
{
	const MeshGeometry geometryA;
	const MeshGeometry geometryB;
	// 0 and 2 of geometry A, 1 of B, and 3 of A but a cutout's batch of its own
	std::vector<MeshDraw> draws = MakeDraws(4, geometryA);
	draws[1].Geometry = &geometryB;

	MeshBatchGrouping grouping;
	grouping.Add(draws[0], nullptr, nullptr);
	grouping.Add(draws[1], nullptr, nullptr);
	grouping.Add(draws[2], nullptr, nullptr);
	grouping.Add(draws[3], &k_materialA, nullptr);
	grouping.Build();

	// One an Add, in their order, as MeshBatches keeps them
	MeshInstanceStream stream(MeshInstanceFormats::ID());
	stream.Build(grouping.GetPlacements(), { 10, 11, 12, (U64(1) << 40) | 13 });
	REQUIRE(stream.GetCount() == 4);

	// Instances: draw 0, draw 2 (joined 0's batch, ahead of 1), draw 1, draw 3
	const std::vector<const MeshDraw*> instanceDraws = InstanceDraws(grouping);
	REQUIRE(instanceDraws.size() == 4);
	REQUIRE(instanceDraws[1] == &draws[2]);
	const U64 expected[] = { 10, 12, 11, (U64(1) << 40) | 13 };
	for (U32 index = 0; index < 4; ++index)
	{
		CAPTURE(index);
		const GraphicsLayouts::MeshIDInstanceLayout instance = ReadIDInstance(stream, index);
		CHECK((static_cast<U64>(instance.ID[1]) << 32 | instance.ID[0]) == expected[index]);
		// And its own transform's translation, so the ID didn't land on another draw's instance
		CHECK(instance.World.WorldColumns[3][0] == instanceDraws[index]->Transform->GetValuePtr()[12]);
	}
}
