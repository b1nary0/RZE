#include <doctest/doctest.h>

#include <TestPaths.h>

#include <Utils/Platform/Filepath.h>

TEST_CASE("A filepath's name, stem and extension")
{
	const Filepath file("Assets/Scenes/Old.Sponza.scene");

	CHECK(file.GetFileName() == "Old.Sponza.scene");
	CHECK(file.GetStem() == "Old.Sponza");
	CHECK(file.GetExtension() == ".scene");

	const Filepath folder("Assets/Scenes");
	CHECK(folder.GetFileName() == "Scenes");
	CHECK(folder.GetExtension().empty());
}

TEST_CASE("A filepath's parent folder")
{
	CHECK(Filepath("Assets/Scenes/Sponza.scene").GetParentFolder().GetRelativePath() == NativePath("Assets/Scenes"));
	CHECK(Filepath("Assets/Scenes").GetParentFolder().GetRelativePath() == "Assets");
	CHECK_FALSE(Filepath("Assets").GetParentFolder().IsValid());
}

TEST_CASE("A filepath reads either separator, and is saved with backslashes on every platform")
{
	// Scene files store backslashes, code writes forward slashes: both name the same file
	CHECK(Filepath(R"(ProjectData\Mesh\Tank.meshasset)") == Filepath("ProjectData/Mesh/Tank.meshasset"));
	CHECK(Filepath(R"(ProjectData\Mesh\Tank.meshasset)").GetRelativePath() == NativePath("ProjectData/Mesh/Tank.meshasset"));

	// So a scene saved on one platform is the same file as one saved on another
	CHECK(Filepath("ProjectData/Mesh/Tank.meshasset").GetSerializedPath() == R"(ProjectData\Mesh\Tank.meshasset)");
	CHECK(Filepath(R"(ProjectData\Mesh\Tank.meshasset)").GetSerializedPath() == R"(ProjectData\Mesh\Tank.meshasset)");
}

TEST_CASE("A filepath is inside a folder it's in or under, ignoring case")
{
	const Filepath scenes("Assets/Scenes");

	CHECK(Filepath("Assets/Scenes").IsInside(scenes));
	CHECK(Filepath("Assets/Scenes/Sponza.scene").IsInside(scenes));
	CHECK(Filepath("assets/scenes/Sub/Sponza.scene").IsInside(scenes));

	// Only whole folder names match
	CHECK_FALSE(Filepath("Assets/Scenes2/Sponza.scene").IsInside(scenes));
	CHECK_FALSE(Filepath("Assets").IsInside(scenes));
	CHECK_FALSE(Filepath().IsInside(scenes));
}

TEST_CASE("Filepaths are equal when their relative paths are the same, and case counts")
{
	CHECK(Filepath("Assets/Scenes/Sponza.scene") == Filepath("Assets/Scenes/Sponza.scene"));
	CHECK(Filepath() == Filepath());
	CHECK(Filepath("Assets/Scenes/Sponza.scene") != Filepath("Assets/Scenes/Other.scene"));
	CHECK(Filepath("Assets/Scenes/Sponza.scene") != Filepath());
	// A rename that only changes case is a change
	CHECK(Filepath("Assets/Scenes/Sponza.scene") != Filepath("Assets/Scenes/sponza.scene"));
}
