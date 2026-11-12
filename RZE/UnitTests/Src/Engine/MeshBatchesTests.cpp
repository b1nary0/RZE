#include <doctest/doctest.h>

#include <Graphics/GraphicsLayouts.h>
#include <Graphics/MaterialOverrides.h>
#include <Graphics/MaterialSlot.h>
#include <Graphics/MeshBatches.h>
#include <Graphics/MeshDraw.h>
#include <Graphics/MeshGeometry.h>

#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector3D.h>

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

	// The draws' instances, batch by batch, in the order they were placed
	std::vector<const MeshDraw*> InstanceDraws(const MeshBatches& batches, const std::vector<MeshDraw>& draws)
	{
		std::vector<const MeshDraw*> instanceDraws;
		for (const GraphicsLayouts::MeshInstanceLayout& instance : batches.GetInstances())
		{
			for (const MeshDraw& draw : draws)
			{
				if (draw.OwnMatrices.Transform == instance.Object.Transform && draw.OwnMatrices.InvTransform == instance.Object.InvTransform)
				{
					instanceDraws.push_back(&draw);
					break;
				}
			}
		}
		return instanceDraws;
	}

	// The batches cover the instances in order, with no gaps
	void CheckContiguous(const MeshBatches& batches)
	{
		U32 next = 0;
		for (const MeshBatches::Batch& batch : batches.GetBatches())
		{
			CHECK(batch.FirstInstance == next);
			CHECK(batch.InstanceCount > 0);
			next += batch.InstanceCount;
		}
		CHECK(next == batches.GetInstances().size());
	}
}

TEST_CASE("Draws with the same bind key and geometry are one batch; either differing splits them")
{
	const MeshGeometry geometryA;
	const MeshGeometry geometryB;
	const std::vector<MeshDraw> drawsA = MakeDraws(3, geometryA);
	const std::vector<MeshDraw> drawsB = MakeDraws(2, geometryB);

	MeshBatches batches;
	batches.Add(drawsA[0], &k_materialA);
	batches.Add(drawsA[1], &k_materialA);
	// Same geometry, another key
	batches.Add(drawsA[2], &k_materialB);
	// Same key, another geometry
	batches.Add(drawsB[0], &k_materialA);
	batches.Add(drawsB[1], &k_materialA);
	batches.Build();

	const std::vector<MeshBatches::Batch>& built = batches.GetBatches();
	REQUIRE(built.size() == 3);
	CHECK(built[0].Draw == &drawsA[0]);
	CHECK(built[0].InstanceCount == 2);
	CHECK(built[1].Draw == &drawsA[2]);
	CHECK(built[1].InstanceCount == 1);
	CHECK(built[2].Draw == &drawsB[0]);
	CHECK(built[2].InstanceCount == 2);
	CheckContiguous(batches);
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
	MeshBatches batches;
	batches.Add(draws[3], nullptr);
	batches.Add(draws[2], nullptr);
	batches.Add(draws[4], nullptr);
	batches.Add(draws[0], nullptr);
	batches.Add(draws[1], nullptr);
	batches.Build();

	const std::vector<MeshBatches::Batch>& built = batches.GetBatches();
	REQUIRE(built.size() == 2);
	CHECK(built[0].Draw == &draws[3]);
	CHECK(built[0].InstanceCount == 2);
	CHECK(built[1].Draw == &draws[2]);
	CHECK(built[1].InstanceCount == 3);
	CheckContiguous(batches);

	const std::vector<const MeshDraw*> instanceDraws = InstanceDraws(batches, draws);
	REQUIRE(instanceDraws.size() == 5);
	CHECK(instanceDraws[0] == &draws[3]);
	CHECK(instanceDraws[1] == &draws[4]);
	CHECK(instanceDraws[2] == &draws[2]);
	CHECK(instanceDraws[3] == &draws[0]);
	CHECK(instanceDraws[4] == &draws[1]);
}

TEST_CASE("Ungrouped draws are batches of their own in the order added, never joined")
{
	const MeshGeometry geometry;
	const std::vector<MeshDraw> draws = MakeDraws(5, geometry);

	MeshBatches batches;
	batches.Add(draws[0], &k_materialA);
	// Same geometry as the grouped draw, and as each other: still apart
	batches.AddUngrouped(draws[3]);
	batches.AddUngrouped(draws[1]);
	batches.AddUngrouped(draws[4]);
	batches.Build();

	const std::vector<MeshBatches::Batch>& built = batches.GetBatches();
	REQUIRE(built.size() == 4);
	CHECK(built[0].Draw == &draws[0]);
	CHECK(built[1].Draw == &draws[3]);
	CHECK(built[2].Draw == &draws[1]);
	CHECK(built[3].Draw == &draws[4]);
	for (const MeshBatches::Batch& batch : built)
	{
		CHECK(batch.InstanceCount == 1);
	}
	CheckContiguous(batches);

	const std::vector<const MeshDraw*> instanceDraws = InstanceDraws(batches, draws);
	REQUIRE(instanceDraws.size() == 4);
	CHECK(instanceDraws[1] == &draws[3]);
	CHECK(instanceDraws[2] == &draws[1]);
	CHECK(instanceDraws[3] == &draws[4]);
}

TEST_CASE("A grouped draw added after an ungrouped one joins its batch ahead of it")
{
	const MeshGeometry geometry;
	const std::vector<MeshDraw> draws = MakeDraws(3, geometry);

	MeshBatches batches;
	batches.Add(draws[0], nullptr);
	batches.AddUngrouped(draws[1]);
	batches.Add(draws[2], nullptr);
	batches.Build();

	const std::vector<MeshBatches::Batch>& built = batches.GetBatches();
	REQUIRE(built.size() == 2);
	CHECK(built[0].Draw == &draws[0]);
	CHECK(built[0].InstanceCount == 2);
	CHECK(built[1].Draw == &draws[1]);
	CheckContiguous(batches);
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

	MeshBatches batches;
	batches.Add(draws[0], nullptr);
	batches.Add(draws[1], nullptr);
	batches.Build();

	const std::vector<GraphicsLayouts::MeshInstanceLayout>& instances = batches.GetInstances();
	REQUIRE(instances.size() == 2);
	CHECK(instances[0].Object.Transform == draws[0].OwnMatrices.Transform);
	CHECK(instances[0].Object.InvTransform == draws[0].OwnMatrices.InvTransform);
	CHECK(instances[1].Object.Transform == objectMatrices.Transform);
	CHECK(instances[1].Object.InvTransform == objectMatrices.InvTransform);
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
	MeshBatches batches;
	for (const MeshDraw& draw : draws)
	{
		batches.Add(draw, &k_materialA);
	}
	batches.Build();

	REQUIRE(batches.GetBatches().size() == 1);
	CHECK(batches.GetBatches()[0].InstanceCount == 3);

	const std::vector<GraphicsLayouts::MeshInstanceLayout>& instances = batches.GetInstances();
	REQUIRE(instances.size() == 3);
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

TEST_CASE("Building again after Clear replaces the last pass's batches and instances")
{
	const MeshGeometry geometry;
	const std::vector<MeshDraw> draws = MakeDraws(4, geometry);

	MeshBatches batches;
	for (const MeshDraw& draw : draws)
	{
		batches.AddUngrouped(draw);
	}
	batches.Build();
	REQUIRE(batches.GetBatches().size() == 4);

	batches.Clear();
	batches.Add(draws[3], nullptr);
	batches.Build();
	REQUIRE(batches.GetBatches().size() == 1);
	REQUIRE(batches.GetInstances().size() == 1);
	CHECK(batches.GetBatches()[0].Draw == &draws[3]);
	CHECK(batches.GetBatches()[0].FirstInstance == 0);
	CHECK(batches.GetInstances()[0].Object.Transform == draws[3].OwnMatrices.Transform);

	batches.Clear();
	batches.Build();
	CHECK(batches.GetBatches().empty());
	CHECK(batches.GetInstances().empty());
}

TEST_CASE("The input layout reads the vertex from slot 0 and the instance's 10 registers from slot 1, 16 bytes apart")
{
	const Rendering::ShaderInputLayout& layout = MeshBatches::GetInputLayout();
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
		CHECK(element.InputSlot == 1);
		CHECK(element.DataClassification == Rendering::EDataClassification::PER_INSTANCE);
		CHECK(element.DataFormat == Rendering::EDataFormat::R32G32B32A32_FLOAT);
		CHECK(element.Offset == column * 16);
		// D3D11 takes semantic index 0 for every element, so a trailing digit wouldn't match the shader's semantic
		CHECK(!element.Name.empty());
		CHECK((element.Name.back() < '0' || element.Name.back() > '9'));
	}
	CHECK(layout[13].Offset + 16 == sizeof(GraphicsLayouts::MeshInstanceLayout));
}
