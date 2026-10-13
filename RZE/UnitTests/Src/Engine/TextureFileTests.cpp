#include <doctest/doctest.h>

#include <Graphics/TextureFile.h>

#include <Utils/Platform/Filepath.h>

#include <STB/stb_image.h>

#include <filesystem>
#include <vector>

namespace
{
	// A scratch folder next to the test executable (Filepaths are relative to it), removed again when the test ends
	class ScopedTempFolder
	{
	public:
		ScopedTempFolder()
			: m_folder("UnitTestsTemp_TextureFile")
		{
			std::filesystem::remove_all(m_folder.GetAbsolutePath());
		}

		~ScopedTempFolder()
		{
			std::error_code ignored;
			std::filesystem::remove_all(m_folder.GetAbsolutePath(), ignored);
		}

		Filepath GetFile(const std::string& relativePath) const
		{
			return Filepath(m_folder.GetRelativePath() + "/" + relativePath);
		}

	private:
		Filepath m_folder;
	};
}

TEST_CASE("WritePNG writes texels that read back the same, creating the folder")
{
	const ScopedTempFolder folder;
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
