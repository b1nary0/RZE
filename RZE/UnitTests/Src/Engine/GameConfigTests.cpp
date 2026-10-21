#include <doctest/doctest.h>

#include <EngineCore/Config/GameConfig.h>

#include <Utils/Platform/File.h>
#include <Utils/Platform/Filepath.h>

#include <TestFolder.h>

#include <fstream>
#include <sstream>
#include <string>

namespace
{
	std::string ReadText(const Filepath& file)
	{
		std::ifstream stream(file.GetAbsolutePath());
		std::stringstream text;
		text << stream.rdbuf();
		return text.str();
	}
}

TEST_CASE("GameConfig saves its icon, keeping the file's comments")
{
	const ScopedTestFolder folder("UnitTestsTemp_GameConfig");
	const Filepath file = folder.GetFile("Game.ini");
	std::ofstream(file.GetAbsolutePath()) << "[Game]\n; The game window's icon\nIcon=\n";

	GameConfig config;
	REQUIRE(config.Load(file));
	CHECK_FALSE(config.GetIcon().IsValid());

	config.SetIcon(Filepath("Assets/Icons/Game.png"));
	REQUIRE(config.Save());
	CHECK_FALSE(File::GetTemporaryPathFor(file).Exists());

	GameConfig reloaded;
	REQUIRE(reloaded.Load(file));
	CHECK(reloaded.GetIcon().GetRelativePath() == Filepath("Assets/Icons/Game.png").GetRelativePath());
	CHECK(ReadText(file).find("; The game window's icon") != std::string::npos);

	// Clearing it saves an empty value, which reads back as no icon
	reloaded.SetIcon(Filepath());
	REQUIRE(reloaded.Save());
	GameConfig cleared;
	REQUIRE(cleared.Load(file));
	CHECK_FALSE(cleared.GetIcon().IsValid());
}

TEST_CASE("GameConfig without a file has no icon, and Save creates the file")
{
	const ScopedTestFolder folder("UnitTestsTemp_GameConfig");
	const Filepath file = folder.GetFile("Game.ini");

	GameConfig config;
	REQUIRE(config.Load(file));
	CHECK_FALSE(config.GetIcon().IsValid());

	config.SetIcon(Filepath("Assets/Icons/Game.png"));
	REQUIRE(config.Save());
	REQUIRE(file.Exists());

	GameConfig reloaded;
	REQUIRE(reloaded.Load(file));
	CHECK(reloaded.GetIcon().GetRelativePath() == Filepath("Assets/Icons/Game.png").GetRelativePath());
}
