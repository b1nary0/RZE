#include <doctest/doctest.h>

#include <Utils/Platform/Filepath.h>
#include <Utils/Platform/Shell.h>

#include <TestFolder.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace
{
	// Points the home trash into a test's folder while it's in scope, then puts XDG_DATA_HOME back
	class ScopedDataHome
	{
	public:
		explicit ScopedDataHome(const Filepath& folder)
			: m_trash(std::filesystem::path(folder.GetAbsolutePath()) / "Trash")
		{
			const char* previous = std::getenv("XDG_DATA_HOME");
			m_hadPrevious = previous != nullptr;
			if (m_hadPrevious)
			{
				m_previous = previous;
			}
			::setenv("XDG_DATA_HOME", folder.GetAbsolutePath().c_str(), 1);
		}

		~ScopedDataHome()
		{
			if (m_hadPrevious)
			{
				::setenv("XDG_DATA_HOME", m_previous.c_str(), 1);
			}
			else
			{
				::unsetenv("XDG_DATA_HOME");
			}
		}

		ScopedDataHome(const ScopedDataHome&) = delete;
		ScopedDataHome& operator=(const ScopedDataHome&) = delete;

		// The trash in it, which MoveToRecycleBin makes
		const std::filesystem::path& GetTrash() const { return m_trash; }

	private:
		std::filesystem::path m_trash;
		std::string m_previous;
		bool m_hadPrevious = false;
	};

	std::vector<std::string> ReadLines(const std::filesystem::path& file)
	{
		std::ifstream stream(file);
		std::vector<std::string> lines;
		for (std::string line; std::getline(stream, line);)
		{
			lines.push_back(line);
		}
		return lines;
	}

	bool EndsWith(const std::string& text, const std::string& end)
	{
		return text.size() >= end.size() && text.compare(text.size() - end.size(), end.size(), end) == 0;
	}

	size_t CountEntries(const std::filesystem::path& folder)
	{
		std::error_code error;
		std::filesystem::directory_iterator iterator(folder, error);
		return error ? 0 : static_cast<size_t>(std::distance(iterator, std::filesystem::directory_iterator()));
	}
}

TEST_CASE("Shell::MoveToRecycleBin moves a file into the Trash, with a .trashinfo saying where it was and when")
{
	const ScopedTestFolder folder("UnitTestsTemp_ShellLinux");
	const ScopedDataHome dataHome(folder.GetFile("Data"));
	const Filepath tank = folder.AddFile("UnitTestsTempTank.meshasset");

	REQUIRE(Shell::MoveToRecycleBin(tank));

	CHECK_FALSE(tank.Exists());
	CHECK(ReadLines(dataHome.GetTrash() / "files" / "UnitTestsTempTank.meshasset") == std::vector<std::string>{ "test" });

	const std::vector<std::string> info = ReadLines(dataHome.GetTrash() / "info" / "UnitTestsTempTank.meshasset.trashinfo");
	REQUIRE(info.size() == 3);
	CHECK(info[0] == "[Trash Info]");
	// Absolute, in the home trash
	CHECK(info[1].rfind("Path=/", 0) == 0);
	CHECK(EndsWith(info[1], "/UnitTestsTemp_ShellLinux/UnitTestsTempTank.meshasset"));
	// Local time, as YYYY-MM-DDThh:mm:ss
	const std::string datePrefix = "DeletionDate=";
	CHECK(info[2].rfind(datePrefix, 0) == 0);
	REQUIRE(info[2].size() == datePrefix.size() + 19);
	CHECK(info[2][datePrefix.size() + 10] == 'T');
}

TEST_CASE("Shell::MoveToRecycleBin numbers a name another trashed item has")
{
	const ScopedTestFolder folder("UnitTestsTemp_ShellLinux");
	const ScopedDataHome dataHome(folder.GetFile("Data"));
	const Filepath tank = folder.AddFile("UnitTestsTempTank.meshasset");
	REQUIRE(Shell::MoveToRecycleBin(tank));
	folder.AddFile("UnitTestsTempTank.meshasset");

	REQUIRE(Shell::MoveToRecycleBin(tank));

	CHECK(std::filesystem::exists(dataHome.GetTrash() / "files" / "UnitTestsTempTank.meshasset"));
	CHECK(std::filesystem::exists(dataHome.GetTrash() / "files" / "UnitTestsTempTank.2.meshasset"));
	const std::vector<std::string> info = ReadLines(dataHome.GetTrash() / "info" / "UnitTestsTempTank.2.meshasset.trashinfo");
	REQUIRE(info.size() == 3);
	// Where it was is unchanged by the number
	CHECK(EndsWith(info[1], "/UnitTestsTemp_ShellLinux/UnitTestsTempTank.meshasset"));
}

TEST_CASE("Shell::MoveToRecycleBin percent-encodes the path in the .trashinfo, but not the name in files/")
{
	const ScopedTestFolder folder("UnitTestsTemp_ShellLinux");
	const ScopedDataHome dataHome(folder.GetFile("Data"));
	// u with an umlaut, spelled out as UTF-8, so it doesn't depend on this file's encoding
	const std::string name = "trash probe \xC3\xBC.txt";
	const Filepath probe = folder.AddFile(name);

	REQUIRE(Shell::MoveToRecycleBin(probe));

	CHECK(std::filesystem::exists(dataHome.GetTrash() / "files" / name));
	const std::vector<std::string> info = ReadLines(dataHome.GetTrash() / "info" / (name + ".trashinfo"));
	REQUIRE(info.size() == 3);
	CHECK(EndsWith(info[1], "/trash%20probe%20%C3%BC.txt"));
}

TEST_CASE("Shell::MoveToRecycleBin takes a folder with everything in it")
{
	const ScopedTestFolder folder("UnitTestsTemp_ShellLinux");
	const ScopedDataHome dataHome(folder.GetFile("Data"));
	const Filepath tank = folder.AddFile("Vehicles/UnitTestsTempTank.meshasset");
	const Filepath vehicles = tank.GetParentFolder();

	REQUIRE(Shell::MoveToRecycleBin(vehicles));

	CHECK_FALSE(vehicles.Exists());
	CHECK(std::filesystem::exists(dataHome.GetTrash() / "files" / "Vehicles" / "UnitTestsTempTank.meshasset"));
	const std::vector<std::string> info = ReadLines(dataHome.GetTrash() / "info" / "Vehicles.trashinfo");
	REQUIRE(info.size() == 3);
	CHECK(EndsWith(info[1], "/UnitTestsTemp_ShellLinux/Vehicles"));
}

TEST_CASE("Shell::MoveToRecycleBin trashes a link as a link, leaving what it points to")
{
	const ScopedTestFolder folder("UnitTestsTemp_ShellLinux");
	const ScopedDataHome dataHome(folder.GetFile("Data"));
	const Filepath target = folder.AddFile("UnitTestsTempTank.meshasset");
	const Filepath link = folder.GetFile("UnitTestsTempLink.meshasset");
	std::filesystem::create_symlink(target.GetAbsolutePath(), link.GetAbsolutePath());

	REQUIRE(Shell::MoveToRecycleBin(link));

	CHECK(target.Exists());
	CHECK(std::filesystem::is_symlink(dataHome.GetTrash() / "files" / "UnitTestsTempLink.meshasset"));
}

TEST_CASE("Shell::MoveToRecycleBin changes nothing for a path that isn't there")
{
	const ScopedTestFolder folder("UnitTestsTemp_ShellLinux");
	const ScopedDataHome dataHome(folder.GetFile("Data"));

	CHECK_FALSE(Shell::MoveToRecycleBin(folder.GetFile("Missing.meshasset")));

	CHECK_FALSE(std::filesystem::exists(dataHome.GetTrash()));
}

TEST_CASE("Shell::MoveToRecycleBin leaves no .trashinfo when the item can't be moved")
{
	// A folder can't be moved into a folder inside itself, even by root (as CI runs), so the trash goes inside the item
	const ScopedTestFolder folder("UnitTestsTemp_ShellLinux");
	folder.AddFile("Project/UnitTestsTempTank.meshasset");
	const Filepath project = folder.GetFile("Project");
	const ScopedDataHome dataHome(folder.GetFile("Project/Data"));

	CHECK_FALSE(Shell::MoveToRecycleBin(project));

	CHECK(folder.GetFile("Project/UnitTestsTempTank.meshasset").Exists());
	// It got as far as making the trash, and its .trashinfo, before the move failed
	REQUIRE(std::filesystem::is_directory(dataHome.GetTrash() / "info"));
	CHECK(CountEntries(dataHome.GetTrash() / "info") == 0);
	CHECK(CountEntries(dataHome.GetTrash() / "files") == 0);
}
