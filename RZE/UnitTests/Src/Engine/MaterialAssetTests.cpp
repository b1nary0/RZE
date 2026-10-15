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

	// A material asset as the burner wrote them in version 1: the header, then as version 0 with the blend mode after
	// the flags
	void WriteVersion1(const Filepath& file, U8 textureFlags)
	{
		std::ofstream stream(file.GetAbsolutePath(), std::ios::binary);
		const auto writeSize = [&stream](size_t size) { stream.write(reinterpret_cast<const char*>(&size), sizeof(size)); };

		const uint16_t version = 1;
		stream.write(reinterpret_cast<const char*>(&k_materialAssetMagic), sizeof(k_materialAssetMagic));
		stream.write(reinterpret_cast<const char*>(&version), sizeof(version));
		writeSize(0); // An empty name
		const MaterialData::MaterialProperties properties;
		stream.write(reinterpret_cast<const char*>(&properties), sizeof(properties));
		stream.write(reinterpret_cast<const char*>(&textureFlags), sizeof(textureFlags));
		const MaterialData::EBlendMode blendMode = MaterialData::EBlendMode::Opaque;
		stream.write(reinterpret_cast<const char*>(&blendMode), sizeof(blendMode));
		writeSize(0); // No textures
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
	written.Tint = Vector3D(0.88f, 0.96f, 1.0f);
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
	CHECK(read.Tint.X() == doctest::Approx(0.88f));
	CHECK(read.Tint.Y() == doctest::Approx(0.96f));
	CHECK(read.Tint.Z() == doctest::Approx(1.0f));
	CHECK(read.TexturePaths == written.TexturePaths);
}

TEST_CASE("A material asset from before tints reads as it drew: its texture as it is, grey without one")
{
	const ScopedTestFolder folder("UnitTestsTemp_MaterialAsset");

	const Filepath textured = folder.GetFile("Textured.materialasset");
	WriteVersion1(textured, MaterialData::TEXTUREFLAG_ALBEDO);
	MaterialAssetImporter texturedImporter;
	REQUIRE(texturedImporter.Import(textured));
	CHECK(texturedImporter.GetMaterialData().Tint.X() == doctest::Approx(1.0f));

	const Filepath untextured = folder.GetFile("Untextured.materialasset");
	WriteVersion1(untextured, MaterialData::TEXTUREFLAG_NONE);
	MaterialAssetImporter untexturedImporter;
	REQUIRE(untexturedImporter.Import(untextured));
	CHECK(untexturedImporter.GetMaterialData().Tint.X() == doctest::Approx(MaterialAssetImporter::k_untexturedGrey));

	// Version 0 too
	const Filepath oldest = folder.GetFile("Oldest.materialasset");
	WriteVersion0(oldest, "Oldest", MaterialData::TEXTUREFLAG_NONE, {});
	MaterialAssetImporter oldestImporter;
	REQUIRE(oldestImporter.Import(oldest));
	CHECK(oldestImporter.GetMaterialData().Tint.Y() == doctest::Approx(MaterialAssetImporter::k_untexturedGrey));
}

TEST_CASE("Every blend mode survives a write and read")
{
	const ScopedTestFolder folder("UnitTestsTemp_MaterialAsset");

	const MaterialData::EBlendMode modes[] =
	{
		MaterialData::EBlendMode::Opaque, MaterialData::EBlendMode::Cutout, MaterialData::EBlendMode::Transparent,
		MaterialData::EBlendMode::Premultiplied, MaterialData::EBlendMode::Additive, MaterialData::EBlendMode::Multiply
	};
	for (const MaterialData::EBlendMode mode : modes)
	{
		CAPTURE(MaterialAssetImporter::GetBlendModeName(mode));

		MaterialData written;
		written.MaterialName = MaterialAssetImporter::GetBlendModeName(mode);
		written.BlendMode = mode;

		const Filepath file = folder.GetFile(written.MaterialName + ".materialasset");
		MaterialAssetWriter writer;
		writer.SetOutputPath(file);
		writer.SetMaterialData(written);
		writer.Write();

		MaterialAssetImporter importer;
		REQUIRE(importer.Import(file));
		CHECK(importer.GetMaterialData().BlendMode == mode);
	}
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
