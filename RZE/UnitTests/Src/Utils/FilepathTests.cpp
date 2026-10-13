#include <doctest/doctest.h>

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
	CHECK(Filepath("Assets/Scenes/Sponza.scene").GetParentFolder().GetRelativePath() == R"(Assets\Scenes)");
	CHECK(Filepath("Assets/Scenes").GetParentFolder().GetRelativePath() == "Assets");
	CHECK_FALSE(Filepath("Assets").GetParentFolder().IsValid());
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
