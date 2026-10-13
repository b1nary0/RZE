#include <doctest/doctest.h>

#include <Services/Thumbnails.h>

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
