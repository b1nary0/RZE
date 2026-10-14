#include <doctest/doctest.h>

#include <Services/Thumbnails.h>

#include <TestFolder.h>

#include <chrono>
#include <filesystem>

using namespace Editor;

TEST_CASE("A thumbnail is the asset's file name as a PNG in the thumbnail folder")
{
	const ThumbnailFolder folder("ProjectData/TestThumbnails");

	CHECK(folder.GetPathFor(Filepath("Assets/Scenes/Sponza.scene")).GetRelativePath() == R"(ProjectData\TestThumbnails\Sponza.png)");
	// Only the last extension goes, and the asset's own folders don't matter
	CHECK(folder.GetPathFor(Filepath("ProjectData/Mesh/Sub/Old.Tank.meshasset")).GetRelativePath() == R"(ProjectData\TestThumbnails\Old.Tank.png)");
}

TEST_CASE("An invalid asset path has no thumbnail")
{
	const ThumbnailFolder folder("ProjectData/TestThumbnails");

	CHECK_FALSE(folder.GetPathFor(Filepath()).IsValid());
}

TEST_CASE("Scenes and meshes have thumbnail folders of their own")
{
	CHECK(Thumbnails::Scenes().GetFolder().GetRelativePath() == R"(ProjectData\SceneThumbnails)");
	CHECK(Thumbnails::Meshes().GetFolder().GetRelativePath() == R"(ProjectData\MeshThumbnails)");
}

TEST_CASE("FindFor picks an asset's image by its extension")
{
	const Filepath scene("Assets/Scenes/Sponza.scene");
	const Filepath mesh("ProjectData/Mesh/Cube.MESHASSET");
	const Filepath image("Assets/2D/Logo.png");

	CHECK(Thumbnails::FindFor(scene).GetRelativePath() == Thumbnails::Scenes().GetPathFor(scene).GetRelativePath());
	CHECK(Thumbnails::FindFor(mesh).GetRelativePath() == Thumbnails::Meshes().GetPathFor(mesh).GetRelativePath());
	// An image is its own thumbnail
	CHECK(Thumbnails::FindFor(image).GetRelativePath() == image.GetRelativePath());

	CHECK_FALSE(Thumbnails::FindFor(Filepath("Assets/Shaders/Line.hlsl")).IsValid());
}

TEST_CASE("Survey counts thumbnails that are up to date, out of date and missing")
{
	const ScopedTestFolder folder("UnitTestsTemp_Thumbnails");
	const ThumbnailFolder images(folder.Get().GetRelativePath() + "/Images");

	// Each image a minute after or before its asset was changed
	auto addImage = [&folder, &images](const Filepath& asset, bool isNewer)
		{
			const Filepath image = folder.AddFile("Images/" + asset.GetStem() + ".png");
			const std::filesystem::file_time_type assetTime = std::filesystem::last_write_time(asset.GetAbsolutePath());
			std::filesystem::last_write_time(image.GetAbsolutePath(), assetTime + std::chrono::minutes(isNewer ? 1 : -1));
		};

	const Filepath current = folder.AddFile("Current.scene");
	const Filepath stale = folder.AddFile("Stale.scene");
	const Filepath alsoCurrent = folder.AddFile("AlsoCurrent.scene");
	const Filepath unrendered = folder.AddFile("Unrendered.scene");
	addImage(current, true);
	addImage(stale, false);
	addImage(alsoCurrent, true);

	const Thumbnails::Coverage coverage = Thumbnails::Survey({ current, stale, alsoCurrent, unrendered }, images);

	CHECK(coverage.UpToDate == 2);
	CHECK(coverage.OutOfDate == 1);
	CHECK(coverage.Missing == 1);
	CHECK(coverage.GetTotal() == 4);
}

TEST_CASE("Survey of no assets counts nothing")
{
	CHECK(Thumbnails::Survey({}, ThumbnailFolder("ProjectData/TestThumbnails")).GetTotal() == 0);
}
