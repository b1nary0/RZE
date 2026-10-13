#include <doctest/doctest.h>

#include <Services/FolderListing.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace Editor;

namespace
{
	// A scratch folder next to the test executable (Filepaths are relative to it), removed again when the test ends
	class ScopedTestFolder
	{
	public:
		ScopedTestFolder()
			: m_folder("UnitTestsTemp_FolderListing")
		{
			std::filesystem::remove_all(m_folder.GetAbsolutePath());
			std::filesystem::create_directories(m_folder.GetAbsolutePath());
		}

		~ScopedTestFolder()
		{
			std::error_code ignored;
			std::filesystem::remove_all(m_folder.GetAbsolutePath(), ignored);
		}

		const Filepath& Get() const { return m_folder; }

		void AddFile(const std::string& name) const
		{
			std::ofstream(std::filesystem::path(m_folder.GetAbsolutePath()) / name) << "test";
		}

		void AddFolder(const std::string& name) const
		{
			std::filesystem::create_directories(std::filesystem::path(m_folder.GetAbsolutePath()) / name);
		}

	private:
		Filepath m_folder;
	};

	std::vector<std::string> GetNames(const std::vector<FolderEntry>& entries)
	{
		std::vector<std::string> names;
		for (const FolderEntry& entry : entries)
		{
			names.push_back(entry.Name);
		}
		return names;
	}
}

TEST_CASE("ListFolder lists subfolders first, then files with the extensions, each sorted by name")
{
	const ScopedTestFolder folder;
	folder.AddFile("b.scene");
	folder.AddFile("A.SCENE");
	folder.AddFile("notes.txt");
	folder.AddFolder("Zeta");
	folder.AddFolder("alpha");

	const std::vector<FolderEntry> entries = ListFolder(folder.Get(), { ".scene" });

	CHECK(GetNames(entries) == std::vector<std::string>{ "alpha", "Zeta", "A.SCENE", "b.scene" });
	CHECK(entries[0].IsFolder);
	CHECK_FALSE(entries[2].IsFolder);
	CHECK(entries[3].Path.GetRelativePath() == folder.Get().GetRelativePath() + "\\b.scene");
}

TEST_CASE("ListFolder lists every file when no extensions are given")
{
	const ScopedTestFolder folder;
	folder.AddFile("a.scene");
	folder.AddFile("b.txt");

	CHECK(GetNames(ListFolder(folder.Get(), {})) == std::vector<std::string>{ "a.scene", "b.txt" });
}

TEST_CASE("ListFolder keeps the entries whose name contains the search, ignoring case")
{
	const ScopedTestFolder folder;
	folder.AddFile("Sponza.scene");
	folder.AddFile("SpaceHangar.scene");
	folder.AddFile("Mountain.scene");
	folder.AddFolder("Spare");

	CHECK(GetNames(ListFolder(folder.Get(), { ".scene" }, "SP")) == std::vector<std::string>{ "Spare", "SpaceHangar.scene", "Sponza.scene" });
	CHECK(ListFolder(folder.Get(), { ".scene" }, "nothing").empty());
}

TEST_CASE("ListFolder of a folder that doesn't exist is empty")
{
	CHECK(ListFolder(Filepath("UnitTestsTemp_NoSuchFolder"), {}).empty());
}

TEST_CASE("FormatTimeAgo describes recent times in words and older ones by date")
{
	using namespace std::chrono;
	const system_clock::time_point now = system_clock::now();

	CHECK(FormatTimeAgo(now - seconds(10), now) == "Just now");
	CHECK(FormatTimeAgo(now - minutes(1), now) == "1 minute ago");
	CHECK(FormatTimeAgo(now - minutes(5), now) == "5 minutes ago");
	CHECK(FormatTimeAgo(now - hours(3), now) == "3 hours ago");
	CHECK(FormatTimeAgo(now - hours(30), now) == "Yesterday");
	CHECK(FormatTimeAgo(now - hours(24 * 4), now) == "4 days ago");
	// A date, e.g. "Mar 02, 2026"
	CHECK(FormatTimeAgo(now - hours(24 * 90), now).find(", ") != std::string::npos);
}
