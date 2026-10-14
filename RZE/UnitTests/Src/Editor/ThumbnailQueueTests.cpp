#include <doctest/doctest.h>

#include <Services/ThumbnailQueue.h>

#include <TestFolder.h>

#include <chrono>
#include <filesystem>
#include <string>

using namespace Editor;

namespace
{
	// Assets with their thumbnails in an Images folder beside them
	struct QueuedAssets
	{
		ScopedTestFolder Folder { "UnitTestsTemp_ThumbnailQueue" };
		ThumbnailQueue Queue { [this](const Filepath& asset) { return Folder.GetFile("Images/" + asset.GetStem() + ".png"); } };

		Filepath AddAsset(const std::string& name) { return Folder.AddFile(name); }

		// Writes asset's thumbnail, a minute before or after the asset was changed
		void AddThumbnail(const Filepath& asset, bool isNewer)
		{
			const Filepath image = Folder.AddFile("Images/" + asset.GetStem() + ".png");
			const std::filesystem::file_time_type assetTime = std::filesystem::last_write_time(asset.GetAbsolutePath());
			const std::chrono::minutes offset(isNewer ? 1 : -1);
			std::filesystem::last_write_time(image.GetAbsolutePath(), assetTime + offset);
		}

		std::string TakeNextName()
		{
			Filepath asset;
			if (!Queue.TakeNext(asset))
			{
				return "";
			}
			Queue.FinishCurrent();
			return asset.GetFileName();
		}
	};
}

TEST_CASE_FIXTURE(QueuedAssets, "Each asset is queued once, in the order asked for")
{
	const Filepath sponza = AddAsset("Sponza.scene");
	const Filepath mountain = AddAsset("Mountain.scene");

	Queue.Add(sponza, false);
	Queue.Add(mountain, false);
	Queue.Add(sponza, false);
	CHECK(Queue.GetQueuedCount() == 2);

	CHECK(TakeNextName() == "Sponza.scene");
	CHECK(TakeNextName() == "Mountain.scene");
	CHECK(TakeNextName().empty());
}

TEST_CASE_FIXTURE(QueuedAssets, "Up-to-date thumbnails are skipped, unless asked for with force")
{
	const Filepath upToDate = AddAsset("UpToDate.scene");
	const Filepath stale = AddAsset("Stale.scene");
	const Filepath forced = AddAsset("Forced.scene");
	AddThumbnail(upToDate, true);
	AddThumbnail(stale, false);
	AddThumbnail(forced, true);

	Queue.Add(upToDate, false);
	Queue.Add(stale, false);
	Queue.Add(forced, false);
	// Asking again with force carries over to the one already queued
	Queue.Add(forced, true);

	CHECK(TakeNextName() == "Stale.scene");
	CHECK(TakeNextName() == "Forced.scene");
	CHECK(TakeNextName().empty());
	CHECK(Queue.TakeSkippedCount() == 1);
	CHECK(Queue.TakeSkippedCount() == 0);
}

TEST_CASE_FIXTURE(QueuedAssets, "An asset asked for while it's being rendered goes back on afterwards, forced")
{
	const Filepath sponza = AddAsset("Sponza.scene");

	Queue.Add(sponza, false);
	Filepath current;
	REQUIRE(Queue.TakeNext(current));
	CHECK(Queue.HasCurrent());

	// e.g. saved while its thumbnail was being rendered
	Queue.Add(sponza, false);
	CHECK(Queue.GetQueuedCount() == 0);

	// Its thumbnail is written, so it's up to date, but it may be of the scene before the save
	AddThumbnail(sponza, true);
	Queue.FinishCurrent();
	CHECK_FALSE(Queue.HasCurrent());
	CHECK(Queue.GetQueuedCount() == 1);
	CHECK(TakeNextName() == "Sponza.scene");
}

TEST_CASE_FIXTURE(QueuedAssets, "Clearing forgets what's queued and the current one")
{
	const Filepath sponza = AddAsset("Sponza.scene");
	const Filepath mountain = AddAsset("Mountain.scene");

	Queue.Add(sponza, false);
	Queue.Add(mountain, false);
	Filepath current;
	REQUIRE(Queue.TakeNext(current));
	Queue.Add(sponza, false);

	Queue.Clear();
	CHECK_FALSE(Queue.HasCurrent());
	CHECK(Queue.GetQueuedCount() == 0);
	Queue.FinishCurrent();
	CHECK(Queue.GetQueuedCount() == 0);
}
