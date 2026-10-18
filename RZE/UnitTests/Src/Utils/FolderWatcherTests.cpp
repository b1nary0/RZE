#include <doctest/doctest.h>

#include <Utils/Platform/FolderWatcher.h>

#include <TestFolder.h>

#include <chrono>
#include <filesystem>
#include <thread>

namespace
{
	// The OS tells of a change soon after it's made, not straight away
	bool WaitForChange(FolderWatcher& watcher, std::chrono::milliseconds timeout = std::chrono::seconds(2))
	{
		const std::chrono::steady_clock::time_point giveUp = std::chrono::steady_clock::now() + timeout;
		while (std::chrono::steady_clock::now() < giveUp)
		{
			if (watcher.PollChanged())
			{
				return true;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		return false;
	}

	// Polls until nothing more comes for a while, so the next check sees only what's made after it
	void DrainChanges(FolderWatcher& watcher)
	{
		while (WaitForChange(watcher, std::chrono::milliseconds(200)))
		{
		}
	}
}

TEST_CASE("FolderWatcher tells of files made, renamed and deleted")
{
	const ScopedTestFolder folder("UnitTestsTemp_FolderWatcher");
	FolderWatcher watcher(folder.Get());
	REQUIRE(watcher.IsWatching());

	CHECK_FALSE(watcher.PollChanged());

	const Filepath tank = folder.AddFile("Tank.meshasset");
	CHECK(WaitForChange(watcher));
	DrainChanges(watcher);

	std::filesystem::rename(tank.GetAbsolutePath(), folder.GetFile("Panzer.meshasset").GetAbsolutePath());
	CHECK(WaitForChange(watcher));
	DrainChanges(watcher);

	std::filesystem::remove(folder.GetFile("Panzer.meshasset").GetAbsolutePath());
	CHECK(WaitForChange(watcher));
}

TEST_CASE("FolderWatcher tells of changes in folders under the one it watches")
{
	const ScopedTestFolder folder("UnitTestsTemp_FolderWatcher");
	folder.AddFolder("Vehicles/Air");
	FolderWatcher watcher(folder.Get());
	REQUIRE(watcher.IsWatching());

	folder.AddFile("Vehicles/Air/Plane.meshasset");

	CHECK(WaitForChange(watcher));
}

TEST_CASE("FolderWatcher watches nothing for a folder that isn't there")
{
	FolderWatcher watcher(Filepath("UnitTestsTemp_NoSuchFolder"));

	CHECK_FALSE(watcher.IsWatching());
	CHECK_FALSE(watcher.PollChanged());
}
