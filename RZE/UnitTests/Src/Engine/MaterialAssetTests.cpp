#include <doctest/doctest.h>

#include <Asset/AssetImport/MaterialAssetImporter.h>
#include <Asset/AssetImport/MaterialAssetWriter.h>

#include <Utils/Platform/Filepath.h>

#include <TestFolder.h>

#include <fstream>
#include <string>
#include <vector>

namespace
{
	using MaterialData = MaterialAssetImporter::MaterialData;

	// A material asset as the burner wrote them before version 1: size, name, properties, flags, textures
	void WriteVersion0(const Filepath& file, const std::string& name, U8 textureFlags, const std::vector<std::string>& texturePaths)
	{
		std::ofstream stream(file.GetAbsolutePath(), std::ios::binary);
		const auto writeSize = [&stream](size_t size) { stream.write(reinterpret_cast<const char*>(&size), sizeof(size)); };
		const auto writeString = [&stream, &writeSize](const std::string& string) { writeSize(string.size()); stream.write(string.data(), string.size()); };

		writeSize(0); // The size, which readers skip
		writeString(name);
		const MaterialData::MaterialProperties properties;
		stream.write(reinterpret_cast<const char*>(&properties), sizeof(properties));
		stream.write(reinterpret_cast<const char*>(&textureFlags), sizeof(textureFlags));
		writeSize(texturePaths.size());
		for (const std::string& path : texturePaths)
		{
			writeString(path);
		}
	}
}

TEST_CASE("A material asset reads back as it was written")
{
	const ScopedTestFolder folder("UnitTestsTemp_MaterialAsset");
	const Filepath file = folder.GetFile("Glass.materialasset");

	MaterialData written;
	written.MaterialName = "Glass";
	written.Properties.Shininess = 64.0f;
	written.Properties.Opacity = 0.25f;
	written.TextureFlags = MaterialData::TEXTUREFLAG_ALBEDO | MaterialData::TEXTUREFLAG_OPACITY;
	written.BlendMode = MaterialData::EBlendMode::Transparent;
	written.TexturePaths = { "Assets/Glass_Colour.png", "", "", "Assets/Glass_Mask.png", "" };

	MaterialAssetWriter writer;
	writer.SetOutputPath(file);
	writer.SetMaterialData(written);
	writer.Write();

	MaterialAssetImporter importer;
	REQUIRE(importer.Import(file));
	const MaterialData read = importer.GetMaterialData();
	CHECK(read.MaterialName == "Glass");
	CHECK(read.Properties.Shininess == doctest::Approx(64.0f));
	CHECK(read.Properties.Opacity == doctest::Approx(0.25f));
	CHECK(read.TextureFlags == written.TextureFlags);
	CHECK(read.BlendMode == MaterialData::EBlendMode::Transparent);
	CHECK(read.TexturePaths == written.TexturePaths);
}

TEST_CASE("A material asset from before blend modes reads as cutout with a mask, opaque without")
{
	const ScopedTestFolder folder("UnitTestsTemp_MaterialAsset");

	const Filepath masked = folder.GetFile("Leaf.materialasset");
	WriteVersion0(masked, "Leaf", MaterialData::TEXTUREFLAG_ALBEDO | MaterialData::TEXTUREFLAG_OPACITY, { "Assets/Leaf.png", "", "", "Assets/Leaf_Mask.png", "" });

	MaterialAssetImporter maskedImporter;
	REQUIRE(maskedImporter.Import(masked));
	CHECK(maskedImporter.GetMaterialData().MaterialName == "Leaf");
	CHECK(maskedImporter.GetMaterialData().BlendMode == MaterialData::EBlendMode::Cutout);
	CHECK(maskedImporter.GetMaterialData().TexturePaths[3] == "Assets/Leaf_Mask.png");

	const Filepath plain = folder.GetFile("Stone.materialasset");
	WriteVersion0(plain, "Stone", MaterialData::TEXTUREFLAG_ALBEDO, { "Assets/Stone.png" });

	MaterialAssetImporter plainImporter;
	REQUIRE(plainImporter.Import(plain));
	CHECK(plainImporter.GetMaterialData().BlendMode == MaterialData::EBlendMode::Opaque);
	CHECK(plainImporter.GetMaterialData().TexturePaths.size() == 1);
}
