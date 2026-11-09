#include <doctest/doctest.h>

#include <Asset/AssetImport/MeshAssetImporter.h>
#include <Asset/AssetImport/MeshAssetWriter.h>
#include <Asset/AssetPaths.h>

#include <Utils/Platform/Filepath.h>

#include <TestFolder.h>

#include <cstddef>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

namespace
{
	// What a version 1 mesh asset holds, laid out as MeshAssetWriter writes it. The writer always writes into the
	// project's mesh folder, so tests write their assets themselves.
	struct MeshAssetLayout
	{
		size_t MeshCount = 1;
		size_t PlacementCount = 1;
		// One more float than the vertices fill, when set
		bool ExtraVertexFloat = false;
	};

	void AppendBytes(std::vector<Byte>& bytes, const void* data, size_t sizeBytes)
	{
		const Byte* const begin = static_cast<const Byte*>(data);
		bytes.insert(bytes.end(), begin, begin + sizeBytes);
	}

	void AppendSize(std::vector<Byte>& bytes, size_t size)
	{
		AppendBytes(bytes, &size, sizeof(size));
	}

	// One triangle, with no material, drawn once
	void WriteMeshAsset(const Filepath& file, const MeshAssetLayout& layout)
	{
		std::vector<Byte> body;
		const std::string name = "Triangle";
		AppendSize(body, name.size());
		AppendBytes(body, name.data(), name.size());
		AppendSize(body, 0); // No material

		std::vector<MeshVertex> vertices(3);
		vertices[1].Position = Vector3D(1.0f, 0.0f, 0.0f);
		vertices[2].Position = Vector3D(0.0f, 1.0f, 0.0f);
		const float extra = 0.0f;
		AppendSize(body, sizeof(MeshVertex) * vertices.size() + (layout.ExtraVertexFloat ? sizeof(float) : 0));
		AppendBytes(body, vertices.data(), sizeof(MeshVertex) * vertices.size());
		if (layout.ExtraVertexFloat)
		{
			AppendBytes(body, &extra, sizeof(extra));
		}

		const U32 indices[] = { 0, 1, 2 };
		AppendSize(body, sizeof(indices));
		AppendBytes(body, indices, sizeof(indices));

		const U32 geometryIndex = 0;
		const float identity[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
		AppendBytes(body, &geometryIndex, sizeof(geometryIndex));
		AppendBytes(body, identity, sizeof(identity));

		MeshAssetFileHeader header;
		std::memset(&header, 0, sizeof(header));
		header.AssetVersion = 1;
		header.BufSize = sizeof(header) + body.size();
		header.MeshCount = layout.MeshCount;
		header.PlacementCount = layout.PlacementCount;

		std::ofstream output(file.GetAbsolutePath(), std::ios::binary | std::ios::trunc);
		output.write(reinterpret_cast<const char*>(&header), sizeof(header));
		output.write(reinterpret_cast<const char*>(body.data()), static_cast<std::streamsize>(body.size()));
	}

	void CheckFailsCleanly(const Filepath& file)
	{
		MeshAssetImporter importer;
		CHECK_FALSE(importer.Import(file));
		CHECK(importer.GetSubmeshes().empty());
		CHECK(importer.GetMaterials().empty());
		CHECK(importer.GetPlacements().empty());
	}
}

TEST_CASE("A mesh asset reads back as it was written")
{
	const ScopedTestFolder folder("UnitTestsTemp_MeshAsset");
	const Filepath file = folder.GetFile("Triangle.meshasset");
	WriteMeshAsset(file, MeshAssetLayout());

	MeshAssetImporter importer;
	REQUIRE(importer.Import(file));
	REQUIRE(importer.GetSubmeshes().size() == 1);
	CHECK(importer.GetSubmeshes()[0].Name == "Triangle");
	CHECK(importer.GetSubmeshes()[0].Vertices.size() == 3);
	CHECK(importer.GetSubmeshes()[0].Vertices[1].Position.X() == doctest::Approx(1.0f));
	CHECK(importer.GetSubmeshes()[0].Indices == std::vector<U32>{ 0, 1, 2 });
	CHECK(importer.GetMaterials().size() == 1);
	REQUIRE(importer.GetPlacements().size() == 1);
	CHECK(importer.GetPlacements()[0].GeometryIndex == 0);
}

TEST_CASE("MeshAssetWriter writes material paths in the serialized form, and the importer names materials from them")
{
	MeshData withMaterial;
	withMaterial.MeshName = "Hull";
	withMaterial.MaterialPath = AssetPaths::GetMaterial("UnitTestsTemp", "Hull_Paint");
	withMaterial.VertexDataArray.resize(3);
	withMaterial.IndexArray = { 0, 1, 2 };
	MeshData withoutMaterial = withMaterial;
	withoutMaterial.MeshName = "Hatch";
	withoutMaterial.MaterialPath = Filepath();

	std::vector<MeshData> meshes{ withMaterial, withoutMaterial };
	MeshAssetWriter writer;
	writer.SetAssetName("UnitTestsTemp_MeshAssetWriter");
	writer.SetMeshData(std::move(meshes));
	CHECK(writer.Write());

	// The writer always writes into the project's mesh folder
	const Filepath file = AssetPaths::GetMesh("UnitTestsTemp_MeshAssetWriter");
	MeshAssetImporter importer;
	CHECK(importer.Import(file));
	const std::vector<MeshAssetImporter::ImportedMaterial>& materials = importer.GetMaterials();
	CHECK(materials.size() == 2);
	if (materials.size() == 2)
	{
		CHECK(materials[0].Path == "ProjectData\\Material\\UnitTestsTemp\\Hull_Paint.materialasset");
		CHECK(materials[0].Name == "Hull_Paint");
		CHECK(materials[1].Path.empty());
		CHECK(materials[1].Name == "Default");
	}

	std::error_code ignored;
	std::filesystem::remove(file.GetAbsolutePath(), ignored);
	RemoveEmptyFolders(file.GetParentFolder());
}

TEST_CASE("MeshAssetWriter returns false when it can't open its file, and leaves what's there")
{
	// A folder where the file would go, in the project's mesh folder, where the writer always writes
	const Filepath file = AssetPaths::GetMesh("UnitTestsTemp_Blocked");
	std::filesystem::create_directories(file.GetAbsolutePath());

	std::vector<MeshData> meshes(1);
	meshes[0].MeshName = "Triangle";
	meshes[0].VertexDataArray.resize(3);
	meshes[0].IndexArray = { 0, 1, 2 };
	MeshAssetWriter writer;
	writer.SetAssetName("UnitTestsTemp_Blocked");
	writer.SetMeshData(std::move(meshes));
	CHECK_FALSE(writer.Write());
	CHECK(std::filesystem::is_directory(file.GetAbsolutePath()));

	std::error_code ignored;
	std::filesystem::remove(file.GetAbsolutePath(), ignored);
	RemoveEmptyFolders(file.GetParentFolder());
}

TEST_CASE("A mesh asset cut short anywhere fails to import, and leaves nothing behind")
{
	const ScopedTestFolder folder("UnitTestsTemp_MeshAsset");
	const Filepath file = folder.GetFile("Triangle.meshasset");
	WriteMeshAsset(file, MeshAssetLayout());

	const Filepath truncated = folder.GetFile("Truncated.meshasset");
	ForEachTruncation(file, truncated, [&truncated](size_t length)
		{
			CAPTURE(length);
			CheckFailsCleanly(truncated);
		});
}

TEST_CASE("A mesh asset whose counts can't fit in the file fails before reserving for them")
{
	const ScopedTestFolder folder("UnitTestsTemp_MeshAsset");
	const Filepath file = folder.GetFile("Corrupt.meshasset");

	// Reserving either would throw, so failing cleanly shows nothing was reserved
	MeshAssetLayout meshes;
	meshes.MeshCount = std::numeric_limits<size_t>::max() / 2;
	WriteMeshAsset(file, meshes);
	CheckFailsCleanly(file);

	MeshAssetLayout placements;
	placements.PlacementCount = std::numeric_limits<size_t>::max() / 2;
	WriteMeshAsset(file, placements);
	CheckFailsCleanly(file);

	// Small enough to reserve, but still more than the file holds
	MeshAssetLayout oneTooMany;
	oneTooMany.MeshCount = 2;
	WriteMeshAsset(file, oneTooMany);
	CheckFailsCleanly(file);
}

TEST_CASE("A mesh asset whose vertex data isn't a whole number of vertices fails")
{
	const ScopedTestFolder folder("UnitTestsTemp_MeshAsset");
	const Filepath file = folder.GetFile("Corrupt.meshasset");

	MeshAssetLayout layout;
	layout.ExtraVertexFloat = true;
	WriteMeshAsset(file, layout);
	CheckFailsCleanly(file);
}
