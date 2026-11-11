#include <doctest/doctest.h>

#include <Services/AssetFiles.h>
#include <Services/Thumbnails.h>

#include <TestFolder.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

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

	// Removes a file, if it's there, when the test ends, and the folders it was in if that leaves them empty
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
			RemoveEmptyFolders(m_file.GetParentFolder());
		}

	private:
		Filepath m_file;
	};

	// Removes what a test left in the trash when it ends, and the trash and the folders above it if that leaves them
	// empty
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
			RemoveEmptyFolders(AssetFiles::GetTrashFolder());
		}

	private:
		const TrashedItem& m_item;
	};

	// Stands in for the Recycle Bin while it's in scope: records the paths sent to it, and removes them, or refuses them
	class FakeRecycleBin
	{
	public:
		explicit FakeRecycleBin(bool accepts)
		{
			s_sent.clear();
			s_accepts = accepts;
		}

		static bool Send(const Filepath& path)
		{
			s_sent.push_back(path.GetRelativePath());
			if (!s_accepts)
			{
				return false;
			}
			std::error_code ignored;
			std::filesystem::remove_all(path.GetAbsolutePath(), ignored);
			return true;
		}

		const std::vector<std::string>& GetSent() const { return s_sent; }

	private:
		static std::vector<std::string> s_sent;
		static bool s_accepts;
	};

	std::vector<std::string> FakeRecycleBin::s_sent;
	bool FakeRecycleBin::s_accepts = true;

	Filepath GetTrashEntry(const TrashedItem& item)
	{
		return item.InTrash.GetParentFolder().GetParentFolder();
	}
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

TEST_CASE("AssetFiles::Recycle sends it from where it was, so the Recycle Bin puts it back there")
{
	const ScopedTestFolder folder("UnitTestsTemp_AssetFiles");
	const Filepath tank = folder.AddFile("UnitTestsTempTank.meshasset");
	const ScopedRemoval thumbnail(WriteThumbnail(tank));
	const TrashedItem item = AssetFiles::MoveToTrash(tank);
	const ScopedTrashCleanup cleanup(item);
	REQUIRE(item.IsInTrash());
	const FakeRecycleBin recycleBin(true);

	CHECK(AssetFiles::Recycle(item, &FakeRecycleBin::Send));

	CHECK(recycleBin.GetSent() == std::vector<std::string>{ tank.GetRelativePath() });
	CHECK_FALSE(tank.Exists());
	CHECK_FALSE(Thumbnails::FindFor(tank).Exists());
	CHECK_FALSE(GetTrashEntry(item).Exists());
}

TEST_CASE("AssetFiles::Recycle sends it from the trash if something else has its name now")
{
	const ScopedTestFolder folder("UnitTestsTemp_AssetFiles");
	const Filepath tank = folder.AddFile("UnitTestsTempTank.meshasset");
	const TrashedItem item = AssetFiles::MoveToTrash(tank);
	const ScopedTrashCleanup cleanup(item);
	REQUIRE(item.IsInTrash());
	folder.AddFile("UnitTestsTempTank.meshasset");
	const FakeRecycleBin recycleBin(true);

	CHECK(AssetFiles::Recycle(item, &FakeRecycleBin::Send));

	CHECK(recycleBin.GetSent() == std::vector<std::string>{ item.InTrash.GetRelativePath() });
	CHECK(tank.Exists());
	CHECK_FALSE(GetTrashEntry(item).Exists());
}

TEST_CASE("AssetFiles::Recycle sends it from the trash if the folder it was in has gone")
{
	const ScopedTestFolder folder("UnitTestsTemp_AssetFiles");
	const Filepath tank = folder.AddFile("Vehicles/UnitTestsTempTank.meshasset");
	const Filepath vehicles = tank.GetParentFolder();
	const TrashedItem item = AssetFiles::MoveToTrash(tank);
	const ScopedTrashCleanup cleanup(item);
	REQUIRE(item.IsInTrash());
	std::filesystem::remove(vehicles.GetAbsolutePath());
	const FakeRecycleBin recycleBin(true);

	CHECK(AssetFiles::Recycle(item, &FakeRecycleBin::Send));

	CHECK(recycleBin.GetSent() == std::vector<std::string>{ item.InTrash.GetRelativePath() });
	CHECK_FALSE(vehicles.Exists());
	CHECK_FALSE(GetTrashEntry(item).Exists());
}

TEST_CASE("AssetFiles::Recycle leaves it in the trash if the Recycle Bin won't take it")
{
	const ScopedTestFolder folder("UnitTestsTemp_AssetFiles");
	const Filepath tank = folder.AddFile("UnitTestsTempTank.meshasset");
	const ScopedRemoval thumbnail(WriteThumbnail(tank));
	const TrashedItem item = AssetFiles::MoveToTrash(tank);
	const ScopedTrashCleanup cleanup(item);
	REQUIRE(item.IsInTrash());
	const FakeRecycleBin recycleBin(false);

	CHECK_FALSE(AssetFiles::Recycle(item, &FakeRecycleBin::Send));

	CHECK(recycleBin.GetSent() == std::vector<std::string>{ tank.GetRelativePath() });
	CHECK_FALSE(tank.Exists());
	CHECK(item.InTrash.Exists());
	REQUIRE(item.Thumbnails.size() == 1);
	CHECK(item.Thumbnails.front().InTrash.Exists());

	// Still whole, so undo can put it back
	REQUIRE(AssetFiles::RestoreFromTrash(item));
	CHECK(tank.Exists());
	CHECK(Thumbnails::FindFor(tank).Exists());
}

TEST_CASE("AssetFiles::EmptyTrash sends each item from where it was, even two that were at the same path")
{
	const ScopedTestFolder folder("UnitTestsTemp_AssetFiles");
	const Filepath tank = folder.AddFile("UnitTestsTempTank.meshasset");
	const TrashedItem first = AssetFiles::MoveToTrash(tank);
	const ScopedTrashCleanup firstCleanup(first);
	folder.AddFile("UnitTestsTempTank.meshasset");
	const TrashedItem second = AssetFiles::MoveToTrash(tank);
	const ScopedTrashCleanup secondCleanup(second);
	REQUIRE(first.IsInTrash());
	REQUIRE(second.IsInTrash());
	const FakeRecycleBin recycleBin(true);

	AssetFiles::EmptyTrash(&FakeRecycleBin::Send);

	CHECK(recycleBin.GetSent() == std::vector<std::string>{ tank.GetRelativePath(), tank.GetRelativePath() });
	CHECK_FALSE(tank.Exists());
	CHECK_FALSE(AssetFiles::GetTrashFolder().Exists());
}

TEST_CASE("AssetFiles::EmptyTrash sends an item from the trash if its entry doesn't say where it was")
{
	const ScopedTestFolder folder("UnitTestsTemp_AssetFiles");
	const Filepath tank = folder.AddFile("UnitTestsTempTank.meshasset");
	const TrashedItem item = AssetFiles::MoveToTrash(tank);
	const ScopedTrashCleanup cleanup(item);
	REQUIRE(item.IsInTrash());
	// As in an entry made before entries recorded it
	const Filepath original(GetTrashEntry(item).GetRelativePath() + "/Original");
	REQUIRE(original.Exists());
	std::filesystem::remove(original.GetAbsolutePath());
	const FakeRecycleBin recycleBin(true);

	AssetFiles::EmptyTrash(&FakeRecycleBin::Send);

	CHECK(recycleBin.GetSent() == std::vector<std::string>{ item.InTrash.GetRelativePath() });
	CHECK_FALSE(tank.Exists());
	CHECK_FALSE(AssetFiles::GetTrashFolder().Exists());
}

TEST_CASE("AssetFiles::EmptyTrash leaves an entry the Recycle Bin won't take, with the item in it")
{
	const ScopedTestFolder folder("UnitTestsTemp_AssetFiles");
	const Filepath tank = folder.AddFile("UnitTestsTempTank.meshasset");
	const TrashedItem item = AssetFiles::MoveToTrash(tank);
	const ScopedTrashCleanup cleanup(item);
	REQUIRE(item.IsInTrash());
	const FakeRecycleBin recycleBin(false);

	AssetFiles::EmptyTrash(&FakeRecycleBin::Send);

	CHECK(recycleBin.GetSent() == std::vector<std::string>{ tank.GetRelativePath() });
	CHECK_FALSE(tank.Exists());
	CHECK(item.InTrash.Exists());
}
