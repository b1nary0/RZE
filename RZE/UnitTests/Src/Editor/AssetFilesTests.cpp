#include <doctest/doctest.h>

#include <Services/AssetFiles.h>
#include <Services/Thumbnails.h>

#include <TestFolder.h>

#include <filesystem>
#include <fstream>

using namespace Editor;

namespace
{
	// Writes a stand-in thumbnail for asset where the editor keeps it
	Filepath WriteThumbnail(const Filepath& asset)
	{
		const Filepath image = Thumbnails::FindFor(asset);
		std::filesystem::create_directories(std::filesystem::path(image.GetAbsolutePath()).parent_path());
		std::ofstream(image.GetAbsolutePath()) << "png";
		return image;
	}

	// Removes a file, if it's there, when the test ends
	class ScopedRemoval
	{
	public:
		explicit ScopedRemoval(const Filepath& file)
			: m_file(file)
		{
		}

		~ScopedRemoval()
		{
			std::error_code ignored;
			std::filesystem::remove(m_file.GetAbsolutePath(), ignored);
		}

	private:
		Filepath m_file;
	};
}

TEST_CASE("AssetFiles::Rename takes a mesh's thumbnail with it")
{
	const ScopedTestFolder folder("UnitTestsTemp_AssetFiles");
	const Filepath tank = folder.AddFile("UnitTestsTempTank.meshasset");
	const Filepath panzer = folder.GetFile("UnitTestsTempPanzer.meshasset");
	const ScopedRemoval oldThumbnail(WriteThumbnail(tank));
	const ScopedRemoval newThumbnail(Thumbnails::FindFor(panzer));

	REQUIRE(AssetFiles::Rename(tank, panzer));

	CHECK(panzer.Exists());
	CHECK_FALSE(tank.Exists());
	CHECK(Thumbnails::FindFor(panzer).Exists());
	CHECK_FALSE(Thumbnails::FindFor(tank).Exists());
}

TEST_CASE("AssetFiles::Rename changes nothing if the name is taken")
{
	const ScopedTestFolder folder("UnitTestsTemp_AssetFiles");
	const Filepath tank = folder.AddFile("UnitTestsTempTank.meshasset");
	const Filepath plane = folder.AddFile("UnitTestsTempPlane.meshasset");
	const ScopedRemoval thumbnail(WriteThumbnail(tank));

	CHECK_FALSE(AssetFiles::Rename(tank, plane));

	CHECK(tank.Exists());
	CHECK(Thumbnails::FindFor(tank).Exists());
}

TEST_CASE("AssetFiles::CreateFolder numbers the folder if the name is taken")
{
	const ScopedTestFolder folder("UnitTestsTemp_AssetFiles");

	const Filepath first = AssetFiles::CreateFolder(folder.Get(), "New Folder");
	const Filepath second = AssetFiles::CreateFolder(folder.Get(), "New Folder");

	CHECK(first.GetFileName() == "New Folder");
	CHECK(second.GetFileName() == "New Folder 2");
	CHECK(std::filesystem::is_directory(second.GetAbsolutePath()));
}
