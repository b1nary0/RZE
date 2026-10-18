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
