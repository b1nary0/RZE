#include <doctest/doctest.h>

#include <Graphics/TextureFile.h>

#include <Utils/Platform/File.h>
#include <Utils/Platform/Filepath.h>

#include <TestFolder.h>

#include <STB/stb_image.h>

#include <filesystem>
#include <vector>

TEST_CASE("WritePNG writes texels that read back the same, creating the folder")
{
	const ScopedTestFolder folder("UnitTestsTemp_TextureFile");
	const Filepath filepath = folder.GetFile("Nested/Folder/Image.png");

	constexpr U32 k_width = 3;
	constexpr U32 k_height = 2;
	std::vector<U8> texels;
	for (U32 i = 0; i < k_width * k_height; ++i)
	{
		texels.push_back(static_cast<U8>(i * 40));
		texels.push_back(static_cast<U8>(255 - i * 40));
		texels.push_back(static_cast<U8>(i * 7));
		texels.push_back(255);
	}

	REQUIRE(TextureFile::WritePNG(filepath, k_width, k_height, texels.data()));
	REQUIRE(filepath.Exists());
	CHECK_FALSE(File::GetTemporaryPathFor(filepath).Exists());

	int width = 0;
	int height = 0;
	int channels = 0;
	stbi_uc* const loaded = stbi_load(filepath.GetAbsolutePath().c_str(), &width, &height, &channels, STBI_rgb_alpha);
	REQUIRE(loaded != nullptr);

	CHECK(width == static_cast<int>(k_width));
	CHECK(height == static_cast<int>(k_height));
	CHECK(std::vector<U8>(loaded, loaded + texels.size()) == texels);

	stbi_image_free(loaded);
}

TEST_CASE("WritePNG replaces an existing file whole")
{
	const ScopedTestFolder folder("UnitTestsTemp_TextureFile");
	const Filepath filepath = folder.AddFile("Image.png");

	const U8 texel[4] = { 10, 20, 30, 255 };
	REQUIRE(TextureFile::WritePNG(filepath, 1, 1, texel));
	CHECK_FALSE(File::GetTemporaryPathFor(filepath).Exists());

	int width = 0;
	int height = 0;
	int channels = 0;
	stbi_uc* const loaded = stbi_load(filepath.GetAbsolutePath().c_str(), &width, &height, &channels, STBI_rgb_alpha);
	REQUIRE(loaded != nullptr);
	CHECK(width == 1);
	CHECK(loaded[2] == 30);
	stbi_image_free(loaded);
}
