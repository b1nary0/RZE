#include <doctest/doctest.h>

#include <Utils/Platform/File.h>

#include <TestFolder.h>

#include <filesystem>

TEST_CASE("File::Move renames a file or a folder")
{
	const ScopedTestFolder folder("UnitTestsTemp_File");
	const Filepath tank = folder.AddFile("Tank.meshasset");
	folder.AddFile("Vehicles/Plane.meshasset");

	CHECK(File::Move(tank, folder.GetFile("Panzer.meshasset")));
	CHECK_FALSE(tank.Exists());
	CHECK(folder.GetFile("Panzer.meshasset").Exists());

	CHECK(File::Move(folder.GetFile("Vehicles"), folder.GetFile("Aircraft")));
	CHECK(folder.GetFile("Aircraft/Plane.meshasset").Exists());
}

TEST_CASE("File::Move leaves both alone when something is already there")
{
	const ScopedTestFolder folder("UnitTestsTemp_File");
	const Filepath tank = folder.AddFile("Tank.meshasset");
	const Filepath plane = folder.AddFile("Plane.meshasset");
	folder.AddFolder("Vehicles");

	CHECK_FALSE(File::Move(tank, plane));
	CHECK(tank.Exists());
	CHECK(plane.Exists());

	CHECK_FALSE(File::Move(tank, folder.GetFile("Vehicles")));
	CHECK(tank.Exists());
}

TEST_CASE("File::Move can change only the case of a name")
{
	const ScopedTestFolder folder("UnitTestsTemp_File");
	const Filepath tank = folder.AddFile("tank.meshasset");

	CHECK(File::Move(tank, folder.GetFile("Tank.meshasset")));

	// The name on disk, as listed, has the new case
	const std::filesystem::directory_iterator listing(folder.Get().GetAbsolutePath());
	CHECK(listing->path().filename().string() == "Tank.meshasset");
}
