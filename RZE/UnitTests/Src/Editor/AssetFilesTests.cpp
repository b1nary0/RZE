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

	// Removes what a test left in the trash when it ends, and the trash itself if that leaves it empty
	class ScopedTrashCleanup
	{
	public:
		explicit ScopedTrashCleanup(const TrashedItem& item)
			: m_item(item)
		{
		}

		~ScopedTrashCleanup()
		{
			std::error_code ignored;
			if (m_item.IsInTrash())
			{
				std::filesystem::remove_all(m_item.InTrash.GetParentFolder().GetParentFolder().GetAbsolutePath(), ignored);
			}
			std::filesystem::remove(AssetFiles::GetTrashFolder().GetAbsolutePath(), ignored);
		}

	private:
		const TrashedItem& m_item;
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

TEST_CASE("AssetFiles::CreateFolder makes a folder, but not over one that's there")
{
	const ScopedTestFolder folder("UnitTestsTemp_AssetFiles");
	const Filepath newFolder = folder.GetFile("New Folder");

	CHECK(AssetFiles::CreateFolder(newFolder));
	CHECK(std::filesystem::is_directory(newFolder.GetAbsolutePath()));
	CHECK_FALSE(AssetFiles::CreateFolder(newFolder));
}

TEST_CASE("AssetFiles::GetWhyInUse refuses the open scene and the folders it's in")
{
	const Filepath scene("Assets/Scenes/Level.scene");

	CHECK_FALSE(AssetFiles::GetWhyInUse(Filepath("Assets/Scenes/LEVEL.scene"), scene).empty());
	CHECK_FALSE(AssetFiles::GetWhyInUse(Filepath("Assets/Scenes"), scene).empty());
	CHECK(AssetFiles::GetWhyInUse(Filepath("Assets/Scenes/Other.scene"), scene).empty());
	CHECK(AssetFiles::GetWhyInUse(Filepath("Assets/Scenes/Level.scene"), Filepath()).empty());
}

TEST_CASE("AssetFiles::MoveToTrash sets a mesh and its thumbnail aside, and RestoreFromTrash puts them back")
{
	const ScopedTestFolder folder("UnitTestsTemp_AssetFiles");
	const Filepath tank = folder.AddFile("UnitTestsTempTank.meshasset");
	const ScopedRemoval thumbnail(WriteThumbnail(tank));

	const TrashedItem item = AssetFiles::MoveToTrash(tank);
	const ScopedTrashCleanup cleanup(item);

	REQUIRE(item.IsInTrash());
	CHECK_FALSE(tank.Exists());
	CHECK_FALSE(Thumbnails::FindFor(tank).Exists());
	CHECK(item.InTrash.Exists());
	CHECK(item.InTrash.GetFileName() == tank.GetFileName());
	CHECK(item.InTrash.IsInside(AssetFiles::GetTrashFolder()));

	REQUIRE(AssetFiles::RestoreFromTrash(item));

	CHECK(tank.Exists());
	CHECK(Thumbnails::FindFor(tank).Exists());
	CHECK_FALSE(item.InTrash.GetParentFolder().GetParentFolder().Exists());
}

TEST_CASE("AssetFiles::MoveToTrash takes a folder with the thumbnails of the meshes in it")
{
	const ScopedTestFolder folder("UnitTestsTemp_AssetFiles");
	const Filepath tank = folder.AddFile("Vehicles/UnitTestsTempTank.meshasset");
	const Filepath vehicles = tank.GetParentFolder();
	const ScopedRemoval thumbnail(WriteThumbnail(tank));

	const TrashedItem item = AssetFiles::MoveToTrash(vehicles);
	const ScopedTrashCleanup cleanup(item);

	REQUIRE(item.IsInTrash());
	CHECK_FALSE(vehicles.Exists());
	CHECK(item.Thumbnails.size() == 1);
	CHECK_FALSE(Thumbnails::FindFor(tank).Exists());

	REQUIRE(AssetFiles::RestoreFromTrash(item));

	CHECK(tank.Exists());
	CHECK(Thumbnails::FindFor(tank).Exists());
}

TEST_CASE("AssetFiles::RestoreFromTrash leaves it in the trash if something else has its name now")
{
	const ScopedTestFolder folder("UnitTestsTemp_AssetFiles");
	const Filepath tank = folder.AddFile("UnitTestsTempTank.meshasset");

	const TrashedItem item = AssetFiles::MoveToTrash(tank);
	const ScopedTrashCleanup cleanup(item);
	REQUIRE(item.IsInTrash());
	folder.AddFile("UnitTestsTempTank.meshasset");

	CHECK_FALSE(AssetFiles::RestoreFromTrash(item));
	CHECK(item.InTrash.Exists());
}

TEST_CASE("AssetFiles::MoveToTrash changes nothing for a path that isn't there")
{
	const ScopedTestFolder folder("UnitTestsTemp_AssetFiles");

	const TrashedItem item = AssetFiles::MoveToTrash(folder.GetFile("Missing.meshasset"));
	const ScopedTrashCleanup cleanup(item);

	CHECK_FALSE(item.IsInTrash());
}
