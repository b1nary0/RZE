#include <doctest/doctest.h>

#include <Services/FolderListing.h>

#include <TestFolder.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace Editor;

namespace
{
	std::vector<std::string> GetNames(const std::vector<FolderEntry>& entries)
	{
		std::vector<std::string> names;
		for (const FolderEntry& entry : entries)
		{
			names.push_back(entry.Name);
		}
		return names;
	}
}

TEST_CASE("ListFolder lists subfolders first, then files with the extensions, each sorted by name")
{
	const ScopedTestFolder folder("UnitTestsTemp_FolderListing");
	folder.AddFile("b.scene");
	folder.AddFile("A.SCENE");
	folder.AddFile("notes.txt");
	folder.AddFolder("Zeta");
	folder.AddFolder("alpha");

	const std::vector<FolderEntry> entries = ListFolder(folder.Get(), { ".scene" });

	CHECK(GetNames(entries) == std::vector<std::string>{ "alpha", "Zeta", "A.SCENE", "b.scene" });
	CHECK(entries[0].IsFolder);
	CHECK_FALSE(entries[2].IsFolder);
	CHECK(entries[3].Path.GetRelativePath() == folder.Get().GetRelativePath() + "\\b.scene");
}

TEST_CASE("ListFolder lists every file when no extensions are given")
{
	const ScopedTestFolder folder("UnitTestsTemp_FolderListing");
	folder.AddFile("a.scene");
	folder.AddFile("b.txt");

	CHECK(GetNames(ListFolder(folder.Get(), {})) == std::vector<std::string>{ "a.scene", "b.txt" });
}

TEST_CASE("ListFolder keeps the entries whose name contains the search, ignoring case")
{
	const ScopedTestFolder folder("UnitTestsTemp_FolderListing");
	folder.AddFile("Sponza.scene");
	folder.AddFile("SpaceHangar.scene");
	folder.AddFile("Mountain.scene");
	folder.AddFolder("Spare");

	CHECK(GetNames(ListFolder(folder.Get(), { ".scene" }, "SP")) == std::vector<std::string>{ "Spare", "SpaceHangar.scene", "Sponza.scene" });
	CHECK(ListFolder(folder.Get(), { ".scene" }, "nothing").empty());
}

TEST_CASE("ListFolder of a folder that doesn't exist is empty")
{
	CHECK(ListFolder(Filepath("UnitTestsTemp_NoSuchFolder"), {}).empty());
}

TEST_CASE("ListSubfolders lists only the folders directly inside, sorted by name ignoring case")
{
	const ScopedTestFolder folder("UnitTestsTemp_FolderListing");
	folder.AddFolder("Zeta");
	folder.AddFolder("alpha");
	folder.AddFolder("alpha/Nested");
	folder.AddFile("Beta.txt");

	std::vector<std::string> paths;
	for (const Filepath& subfolder : ListSubfolders(folder.Get()))
	{
		paths.push_back(subfolder.GetRelativePath());
	}

	const std::string root = folder.Get().GetRelativePath();
	CHECK(paths == std::vector<std::string>{ root + R"(\alpha)", root + R"(\Zeta)" });
}

TEST_CASE("ListSubfolders of a folder that doesn't exist is empty")
{
	CHECK(ListSubfolders(Filepath("UnitTestsTemp_NoSuchFolder")).empty());
}

TEST_CASE("FormatTimeAgo describes recent times in words and older ones by date")
{
	using namespace std::chrono;
	const system_clock::time_point now = system_clock::now();

	CHECK(FormatTimeAgo(now - seconds(10), now) == "Just now");
	CHECK(FormatTimeAgo(now - minutes(1), now) == "1 minute ago");
	CHECK(FormatTimeAgo(now - minutes(5), now) == "5 minutes ago");
	CHECK(FormatTimeAgo(now - hours(3), now) == "3 hours ago");
	CHECK(FormatTimeAgo(now - hours(30), now) == "Yesterday");
	CHECK(FormatTimeAgo(now - hours(24 * 4), now) == "4 days ago");
	// A date, e.g. "Mar 02, 2026"
	CHECK(FormatTimeAgo(now - hours(24 * 90), now).find(", ") != std::string::npos);
}

TEST_CASE("ParseExtensionFilter turns a file-prompt filter into extensions")
{
	CHECK(ParseExtensionFilter("*.meshasset") == std::vector<std::string>{ ".meshasset" });
	CHECK(ParseExtensionFilter("*.scene;*.meshasset") == std::vector<std::string>{ ".scene", ".meshasset" });
	CHECK(ParseExtensionFilter(".png") == std::vector<std::string>{ ".png" });

	// Any file
	CHECK(ParseExtensionFilter("*.*").empty());
	CHECK(ParseExtensionFilter("").empty());
}

TEST_CASE("ResolveSaveName adds the extension unless the name already ends with it")
{
	const Filepath scenes("Assets/Scenes");

	CHECK(ResolveSaveName(scenes, "Level 2", ".scene").Path.GetRelativePath() == R"(Assets\Scenes\Level 2.scene)");
	CHECK(ResolveSaveName(scenes, "Level 2.scene", ".scene").Path.GetRelativePath() == R"(Assets\Scenes\Level 2.scene)");
	CHECK(ResolveSaveName(scenes, "Level 2.SCENE", ".scene").Path.GetRelativePath() == R"(Assets\Scenes\Level 2.SCENE)");
	// Another extension is part of the name
	CHECK(ResolveSaveName(scenes, "notes.txt", ".scene").Path.GetRelativePath() == R"(Assets\Scenes\notes.txt.scene)");
	// Spaces around the name go
	CHECK(ResolveSaveName(scenes, "  Level 2 ", ".scene").Path.GetRelativePath() == R"(Assets\Scenes\Level 2.scene)");
	CHECK(ResolveSaveName(scenes, "Level 2", ".scene").Error.empty());
}

TEST_CASE("ResolveSaveName refuses names Windows can't use")
{
	const Filepath scenes("Assets/Scenes");

	for (const char* name : { "", "   ", "a/b", R"(a\b)", "a:b", "a*b", "a?b", "a\"b", "a<b", "a>b", "a|b", "trailing." })
	{
		const ResolvedName saveName = ResolveSaveName(scenes, name, ".scene");
		CHECK_MESSAGE(!saveName.Path.IsValid(), name);
		CHECK_MESSAGE(!saveName.Error.empty(), name);
	}
}

TEST_CASE("GetFileNameError refuses Windows' device names, whatever the extension")
{
	for (const char* name : { "CON", "con", "Nul.scene", "COM1", "lpt9.txt", "AUX .scene" })
	{
		CHECK_MESSAGE(!GetFileNameError(name).empty(), name);
	}
	for (const char* name : { "CONSOLE", "Auxiliary.scene", "COM10", "Level 2.scene", "my.con" })
	{
		CHECK_MESSAGE(GetFileNameError(name).empty(), name);
	}
}

TEST_CASE("ResolveRename keeps a file's extension, and takes a folder's name as it is")
{
	const ScopedTestFolder folder("UnitTestsTemp_FolderListing");
	const Filepath mesh = folder.AddFile("Tank.meshasset");
	folder.AddFolder("Vehicles");
	const std::string root = folder.Get().GetRelativePath();

	CHECK(ResolveRename(mesh, false, "Panzer").Path.GetRelativePath() == root + R"(\Panzer.meshasset)");
	CHECK(ResolveRename(mesh, false, "Panzer.meshasset").Path.GetRelativePath() == root + R"(\Panzer.meshasset)");
	CHECK(ResolveRename(mesh, false, "  Panzer ").Path.GetRelativePath() == root + R"(\Panzer.meshasset)");
	CHECK(ResolveRename(folder.GetFile("Vehicles"), true, "Cars.v2").Path.GetRelativePath() == root + R"(\Cars.v2)");
}

TEST_CASE("ResolveRename refuses a name something else has, but not a change of case")
{
	const ScopedTestFolder folder("UnitTestsTemp_FolderListing");
	const Filepath tank = folder.AddFile("Tank.meshasset");
	folder.AddFile("Plane.meshasset");
	folder.AddFolder("Vehicles");

	const ResolvedName taken = ResolveRename(tank, false, "plane");
	CHECK_FALSE(taken.Path.IsValid());
	CHECK_FALSE(taken.Error.empty());
	CHECK_FALSE(ResolveRename(folder.GetFile("Vehicles"), true, "Plane.meshasset").Path.IsValid());

	CHECK(ResolveRename(tank, false, "TANK").Error.empty());
	CHECK(ResolveRename(tank, false, "Tank").Error.empty());
	CHECK_FALSE(ResolveRename(tank, false, "a:b").Path.IsValid());
}

TEST_CASE("MakeUniqueFileName numbers a name that's taken, before its extension")
{
	const ScopedTestFolder folder("UnitTestsTemp_FolderListing");
	const std::string root = folder.Get().GetRelativePath();

	CHECK(MakeUniqueFileName(folder.Get(), "New Folder").GetRelativePath() == root + R"(\New Folder)");

	folder.AddFolder("New Folder");
	folder.AddFolder("New Folder 2");
	CHECK(MakeUniqueFileName(folder.Get(), "New Folder").GetRelativePath() == root + R"(\New Folder 3)");

	folder.AddFile("Level.scene");
	CHECK(MakeUniqueFileName(folder.Get(), "Level.scene").GetRelativePath() == root + R"(\Level 2.scene)");
}

TEST_CASE("FindFiles finds files with the extension in subfolders too, sorted by path")
{
	const ScopedTestFolder folder("UnitTestsTemp_FolderListing");
	folder.AddFile("b.meshasset");
	folder.AddFile("Vehicles/Tank.MESHASSET");
	folder.AddFile("Vehicles/Air/Plane.meshasset");
	folder.AddFile("A.meshasset");
	folder.AddFile("notes.txt");
	folder.AddFolder("Empty");

	std::vector<std::string> paths;
	for (const Filepath& file : FindFiles(folder.Get(), ".meshasset"))
	{
		paths.push_back(file.GetRelativePath());
	}

	const std::string root = folder.Get().GetRelativePath();
	CHECK(paths == std::vector<std::string>{ root + R"(\A.meshasset)", root + R"(\b.meshasset)", root + R"(\Vehicles\Air\Plane.meshasset)", root + R"(\Vehicles\Tank.MESHASSET)" });
}

TEST_CASE("FindFiles finds nothing in a folder that doesn't exist")
{
	CHECK(FindFiles(Filepath("UnitTestsTemp_NoSuchFolder"), ".scene").empty());
}
