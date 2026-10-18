#include <doctest/doctest.h>

#include <Utils/Platform/Folders.h>

#include <TestFolder.h>

#include <string>
#include <vector>

TEST_CASE("Folders::ListSubfolders lists only the folders directly inside, sorted by name ignoring case")
{
	const ScopedTestFolder folder("UnitTestsTemp_Folders");
	folder.AddFolder("Zeta");
	folder.AddFolder("alpha");
	folder.AddFolder("alpha/Nested");
	folder.AddFile("Beta.txt");

	std::vector<std::string> paths;
	for (const Filepath& subfolder : Folders::ListSubfolders(folder.Get()))
	{
		paths.push_back(subfolder.GetRelativePath());
	}

	const std::string root = folder.Get().GetRelativePath();
	CHECK(paths == std::vector<std::string>{ root + R"(\alpha)", root + R"(\Zeta)" });
}

TEST_CASE("Folders::ListSubfolders of a folder that doesn't exist is empty")
{
	CHECK(Folders::ListSubfolders(Filepath("UnitTestsTemp_NoSuchFolder")).empty());
}

TEST_CASE("Folders::FindFiles finds files with the extension in subfolders too, sorted by path")
{
	const ScopedTestFolder folder("UnitTestsTemp_Folders");
	folder.AddFile("b.meshasset");
	folder.AddFile("Vehicles/Tank.MESHASSET");
	folder.AddFile("Vehicles/Air/Plane.meshasset");
	folder.AddFile("A.meshasset");
	folder.AddFile("notes.txt");
	folder.AddFolder("Empty");

	std::vector<std::string> paths;
	for (const Filepath& file : Folders::FindFiles(folder.Get(), ".meshasset"))
	{
		paths.push_back(file.GetRelativePath());
	}

	const std::string root = folder.Get().GetRelativePath();
	CHECK(paths == std::vector<std::string>{ root + R"(\A.meshasset)", root + R"(\b.meshasset)", root + R"(\Vehicles\Air\Plane.meshasset)", root + R"(\Vehicles\Tank.MESHASSET)" });
}

TEST_CASE("Folders::FindFiles finds nothing in a folder that doesn't exist")
{
	CHECK(Folders::FindFiles(Filepath("UnitTestsTemp_NoSuchFolder"), ".scene").empty());
}
