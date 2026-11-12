#include <doctest/doctest.h>

#include <Graphics/GraphicsLayouts.h>
#include <Graphics/MeshBatches.h>
#include <Graphics/MeshDraw.h>

#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector3D.h>

#include <vector>

namespace
{
	// Draws placed at different positions, each with its own matrices, as a sub-mesh away from its mesh's origin is
	std::vector<MeshDraw> MakeDraws(size_t count)
	{
		std::vector<MeshDraw> draws(count);
		for (size_t index = 0; index < count; ++index)
		{
			MeshDraw& draw = draws[index];
			const float offset = static_cast<float>(index + 1);
			draw.OwnMatrices.Transform = Matrix4x4::CreateInPlace(Vector3D(offset, 2.0f * offset, -offset), Vector3D(1.0f), Vector3D(0.0f, 10.0f * offset, 0.0f));
			draw.OwnMatrices.InvTransform = draw.OwnMatrices.Transform.Inverse();
			draw.Transform = &draw.OwnMatrices.Transform;
		}
		return draws;
	}
}

TEST_CASE("Each draw is a batch of one instance, in the order given, with its transform and inverse")
{
	const std::vector<MeshDraw> meshDraws = MakeDraws(3);
	// Not the order they're stored in, as forward's blended draws come sorted
	const std::vector<const MeshDraw*> draws = { &meshDraws[2], &meshDraws[0], &meshDraws[1] };

	std::vector<GraphicsLayouts::ObjectBufferLayout> instances;
	std::vector<MeshBatches::Batch> batches;
	MeshBatches::Build(draws, instances, batches);

	REQUIRE(instances.size() == 3);
	REQUIRE(batches.size() == 3);
	for (size_t index = 0; index < draws.size(); ++index)
	{
		CAPTURE(index);
		CHECK(batches[index].Draw == draws[index]);
		CHECK(batches[index].FirstInstance == index);
		CHECK(batches[index].InstanceCount == 1);
		CHECK(instances[index].Transform == draws[index]->OwnMatrices.Transform);
		CHECK(instances[index].InvTransform == draws[index]->OwnMatrices.InvTransform);
	}
}

TEST_CASE("A draw sharing its object's matrices gets them as its instance")
{
	// As a sub-mesh at its mesh's origin: Transform points at the object's ObjectBufferLayout, not the draw's own
	GraphicsLayouts::ObjectBufferLayout objectMatrices;
	objectMatrices.Transform = Matrix4x4::CreateInPlace(Vector3D(5.0f, 0.0f, 0.0f), Vector3D(2.0f), Vector3D(0.0f));
	objectMatrices.InvTransform = objectMatrices.Transform.Inverse();

	std::vector<MeshDraw> meshDraws(2);
	for (MeshDraw& draw : meshDraws)
	{
		draw.Transform = &objectMatrices.Transform;
	}

	std::vector<GraphicsLayouts::ObjectBufferLayout> instances;
	std::vector<MeshBatches::Batch> batches;
	MeshBatches::Build({ &meshDraws[0], &meshDraws[1] }, instances, batches);

	REQUIRE(instances.size() == 2);
	for (const GraphicsLayouts::ObjectBufferLayout& instance : instances)
	{
		CHECK(instance.Transform == objectMatrices.Transform);
		CHECK(instance.InvTransform == objectMatrices.InvTransform);
	}
}

TEST_CASE("Building again replaces the last pass's instances and batches")
{
	const std::vector<MeshDraw> meshDraws = MakeDraws(4);

	std::vector<GraphicsLayouts::ObjectBufferLayout> instances;
	std::vector<MeshBatches::Batch> batches;
	MeshBatches::Build({ &meshDraws[0], &meshDraws[1], &meshDraws[2], &meshDraws[3] }, instances, batches);
	MeshBatches::Build({ &meshDraws[3] }, instances, batches);

	REQUIRE(instances.size() == 1);
	REQUIRE(batches.size() == 1);
	CHECK(batches[0].Draw == &meshDraws[3]);
	CHECK(batches[0].FirstInstance == 0);
	CHECK(instances[0].Transform == meshDraws[3].OwnMatrices.Transform);

	MeshBatches::Build({}, instances, batches);
	CHECK(instances.empty());
	CHECK(batches.empty());
}

TEST_CASE("The input layout reads the vertex from slot 0 and the instance's 8 columns from slot 1, 16 bytes apart")
{
	const Rendering::ShaderInputLayout& layout = MeshBatches::GetInputLayout();
	REQUIRE(layout.GetDataCount() == 12);

	for (size_t index = 0; index < 4; ++index)
	{
		CAPTURE(index);
		CHECK(layout[index].InputSlot == 0);
		CHECK(layout[index].DataClassification == Rendering::EDataClassification::PER_VERTEX);
	}

	for (size_t column = 0; column < 8; ++column)
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
	CHECK(layout[11].Offset + 16 == sizeof(GraphicsLayouts::ObjectBufferLayout));
}
