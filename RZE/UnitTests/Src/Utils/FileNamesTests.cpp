#include <doctest/doctest.h>

#include <Utils/Platform/FileNames.h>

#include <TestFolder.h>

#include <string>
#include <vector>

TEST_CASE("FileNames::ParseExtensionFilter turns a file-prompt filter into extensions")
{
	CHECK(FileNames::ParseExtensionFilter("*.meshasset") == std::vector<std::string>{ ".meshasset" });
	CHECK(FileNames::ParseExtensionFilter("*.scene;*.meshasset") == std::vector<std::string>{ ".scene", ".meshasset" });
	CHECK(FileNames::ParseExtensionFilter(".png") == std::vector<std::string>{ ".png" });

	// Any file
	CHECK(FileNames::ParseExtensionFilter("*.*").empty());
	CHECK(FileNames::ParseExtensionFilter("").empty());
}

TEST_CASE("FileNames::MatchesExtensions matches a name or path ending with one of the extensions, ignoring case")
{
	const std::vector<std::string> meshesAndScenes{ ".meshasset", ".scene" };

	CHECK(FileNames::MatchesExtensions("Tank.meshasset", meshesAndScenes));
	CHECK(FileNames::MatchesExtensions(R"(Assets\Scenes\Level.SCENE)", meshesAndScenes));
	CHECK_FALSE(FileNames::MatchesExtensions("notes.txt", meshesAndScenes));
	// The whole extension, dot and all
	CHECK_FALSE(FileNames::MatchesExtensions("Level.myscene", meshesAndScenes));
	CHECK_FALSE(FileNames::MatchesExtensions("meshasset", meshesAndScenes));

	// No extensions allows any file
	CHECK(FileNames::MatchesExtensions("notes.txt", {}));
}

TEST_CASE("FileNames::ResolveSave adds the extension unless the name already ends with it")
{
	const Filepath scenes("Assets/Scenes");

	CHECK(FileNames::ResolveSave(scenes, "Level 2", ".scene").Path.GetRelativePath() == R"(Assets\Scenes\Level 2.scene)");
	CHECK(FileNames::ResolveSave(scenes, "Level 2.scene", ".scene").Path.GetRelativePath() == R"(Assets\Scenes\Level 2.scene)");
	CHECK(FileNames::ResolveSave(scenes, "Level 2.SCENE", ".scene").Path.GetRelativePath() == R"(Assets\Scenes\Level 2.SCENE)");
	// Another extension is part of the name
	CHECK(FileNames::ResolveSave(scenes, "notes.txt", ".scene").Path.GetRelativePath() == R"(Assets\Scenes\notes.txt.scene)");
	// Spaces around the name go
	CHECK(FileNames::ResolveSave(scenes, "  Level 2 ", ".scene").Path.GetRelativePath() == R"(Assets\Scenes\Level 2.scene)");
	CHECK(FileNames::ResolveSave(scenes, "Level 2", ".scene").Error.empty());
}

TEST_CASE("FileNames::ResolveSave refuses names Windows can't use")
{
	const Filepath scenes("Assets/Scenes");

	for (const char* name : { "", "   ", "a/b", R"(a\b)", "a:b", "a*b", "a?b", "a\"b", "a<b", "a>b", "a|b", "trailing." })
	{
		const FileNames::ResolvedName saveName = FileNames::ResolveSave(scenes, name, ".scene");
		CHECK_MESSAGE(!saveName.Path.IsValid(), name);
		CHECK_MESSAGE(!saveName.Error.empty(), name);
	}
}

TEST_CASE("FileNames::GetError refuses Windows' device names, whatever the extension")
{
	for (const char* name : { "CON", "con", "Nul.scene", "COM1", "lpt9.txt", "AUX .scene" })
	{
		CHECK_MESSAGE(!FileNames::GetError(name).empty(), name);
	}
	for (const char* name : { "CONSOLE", "Auxiliary.scene", "COM10", "Level 2.scene", "my.con" })
	{
		CHECK_MESSAGE(FileNames::GetError(name).empty(), name);
	}
}

TEST_CASE("FileNames::ResolveRename keeps a file's extension, and takes a folder's name as it is")
{
	const ScopedTestFolder folder("UnitTestsTemp_FileNames");
	const Filepath mesh = folder.AddFile("Tank.meshasset");
	folder.AddFolder("Vehicles");
	const std::string root = folder.Get().GetRelativePath();

	CHECK(FileNames::ResolveRename(mesh, false, "Panzer").Path.GetRelativePath() == root + R"(\Panzer.meshasset)");
	CHECK(FileNames::ResolveRename(mesh, false, "Panzer.meshasset").Path.GetRelativePath() == root + R"(\Panzer.meshasset)");
	CHECK(FileNames::ResolveRename(mesh, false, "  Panzer ").Path.GetRelativePath() == root + R"(\Panzer.meshasset)");
	CHECK(FileNames::ResolveRename(folder.GetFile("Vehicles"), true, "Cars.v2").Path.GetRelativePath() == root + R"(\Cars.v2)");
}

TEST_CASE("FileNames::ResolveRename refuses a name something else has, but not a change of case")
{
	const ScopedTestFolder folder("UnitTestsTemp_FileNames");
	const Filepath tank = folder.AddFile("Tank.meshasset");
	folder.AddFile("Plane.meshasset");
	folder.AddFolder("Vehicles");

	const FileNames::ResolvedName taken = FileNames::ResolveRename(tank, false, "plane");
	CHECK_FALSE(taken.Path.IsValid());
	CHECK_FALSE(taken.Error.empty());
	CHECK_FALSE(FileNames::ResolveRename(folder.GetFile("Vehicles"), true, "Plane.meshasset").Path.IsValid());

	CHECK(FileNames::ResolveRename(tank, false, "TANK").Error.empty());
	CHECK(FileNames::ResolveRename(tank, false, "Tank").Error.empty());
	CHECK_FALSE(FileNames::ResolveRename(tank, false, "a:b").Path.IsValid());
}

TEST_CASE("FileNames::MakeUnique numbers a name that's taken, before its extension")
{
	const ScopedTestFolder folder("UnitTestsTemp_FileNames");
	const std::string root = folder.Get().GetRelativePath();

	CHECK(FileNames::MakeUnique(folder.Get(), "New Folder").GetRelativePath() == root + R"(\New Folder)");

	folder.AddFolder("New Folder");
	folder.AddFolder("New Folder 2");
	CHECK(FileNames::MakeUnique(folder.Get(), "New Folder").GetRelativePath() == root + R"(\New Folder 3)");

	folder.AddFile("Level.scene");
	CHECK(FileNames::MakeUnique(folder.Get(), "Level.scene").GetRelativePath() == root + R"(\Level 2.scene)");
}
