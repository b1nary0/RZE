#include <doctest/doctest.h>

#include <Services/FolderNavigator.h>

#include <TestFolder.h>

#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

using namespace Editor;

namespace
{
	std::vector<std::string> GetNames(const std::vector<FolderEntry>& entries)
	{
		std::vector<std::string> names;
		for (const FolderEntry& entry : entries)
		{
			names.push_back(entry.Name);
		}
		return names;
	}

	void SetSearch(FolderNavigator& navigator, const char* text)
	{
		strncpy_s(navigator.GetSearchBuffer(), navigator.GetSearchBufferSize(), text, _TRUNCATE);
		navigator.Refresh();
	}

	// Root
	//   A
	//     A1
	//   B
	struct TestTree
	{
		TestTree()
			: Root("UnitTestsTemp_FolderNavigator")
		{
			Root.AddFolder("A/A1");
			Root.AddFolder("B");
			Root.AddFile("A/notes.txt");
			Root.AddFile("A/level.scene");
		}

		Filepath Get(const std::string& relativePath) const { return Root.GetFile(relativePath); }

		ScopedTestFolder Root;
	};
}

TEST_CASE("Navigating, then going back and forward, retraces the folders")
{
	TestTree tree;
	FolderNavigator navigator;
	navigator.Reset({ tree.Root.Get() }, {});
	CHECK(navigator.GetFolder() == tree.Root.Get());
	CHECK_FALSE(navigator.CanGoBack());

	navigator.Navigate(tree.Get("A"));
	navigator.Navigate(tree.Get("A/A1"));
	CHECK(navigator.GetFolder() == tree.Get("A/A1"));

	navigator.GoBack();
	CHECK(navigator.GetFolder() == tree.Get("A"));
	navigator.GoBack();
	CHECK(navigator.GetFolder() == tree.Root.Get());
	CHECK_FALSE(navigator.CanGoBack());

	navigator.GoForward();
	CHECK(navigator.GetFolder() == tree.Get("A"));
	CHECK(navigator.CanGoForward());

	// Going somewhere new drops what was ahead
	navigator.Navigate(tree.Get("B"));
	CHECK_FALSE(navigator.CanGoForward());

	// The folder shown already: nothing changes, not even the history
	navigator.Navigate(tree.Get("B"));
	navigator.GoBack();
	CHECK(navigator.GetFolder() == tree.Get("A"));
}

TEST_CASE("Up stops at a root and says which folder it came out of")
{
	TestTree tree;
	FolderNavigator navigator;
	navigator.Reset({ tree.Root.Get() }, {}, tree.Get("A/A1"));
	CHECK(navigator.GetFolder() == tree.Get("A/A1"));

	CHECK(navigator.GoUp() == tree.Get("A/A1"));
	CHECK(navigator.GetFolder() == tree.Get("A"));
	CHECK(navigator.GoUp() == tree.Get("A"));
	CHECK(navigator.GetFolder() == tree.Root.Get());

	CHECK_FALSE(navigator.CanGoUp());
	CHECK_FALSE(navigator.GoUp().IsValid());
	CHECK(navigator.GetFolder() == tree.Root.Get());

	// Up counts as going somewhere, so back returns
	navigator.GoBack();
	CHECK(navigator.GetFolder() == tree.Get("A"));
}

TEST_CASE("With two roots, the root is the one the folder shown is in")
{
	TestTree tree;
	FolderNavigator navigator;
	navigator.Reset({ tree.Get("A"), tree.Get("B") }, {}, tree.Get("A/A1"));
	CHECK(navigator.GetRoot() == tree.Get("A"));
	CHECK(navigator.IsRoot(tree.Get("B")));
	CHECK_FALSE(navigator.IsRoot(tree.Get("A/A1")));

	navigator.Navigate(tree.Get("B"));
	CHECK(navigator.GetRoot() == tree.Get("B"));
	CHECK_FALSE(navigator.CanGoUp());

	// A start folder outside every root starts at the first
	navigator.Reset({ tree.Get("A"), tree.Get("B") }, {}, tree.Root.Get());
	CHECK(navigator.GetFolder() == tree.Get("A"));
}

TEST_CASE("Search and extensions filter the entries, and going to another folder clears the search")
{
	TestTree tree;
	FolderNavigator navigator;
	navigator.Reset({ tree.Root.Get() }, {}, tree.Get("A"));
	CHECK(GetNames(navigator.GetEntries()) == std::vector<std::string>{ "A1", "level.scene", "notes.txt" });

	SetSearch(navigator, "NOTE");
	CHECK(navigator.HasSearch());
	CHECK(GetNames(navigator.GetEntries()) == std::vector<std::string>{ "notes.txt" });
	REQUIRE(navigator.FindEntry(tree.Get("A/notes.txt")) != nullptr);
	CHECK(navigator.FindEntry(tree.Get("A/level.scene")) == nullptr);

	navigator.Navigate(tree.Get("A/A1"));
	CHECK_FALSE(navigator.HasSearch());

	navigator.Reset({ tree.Root.Get() }, { ".scene" }, tree.Get("A"));
	CHECK(GetNames(navigator.GetEntries()) == std::vector<std::string>{ "A1", "level.scene" });
}

TEST_CASE("Refreshing after the folder shown is deleted shows the nearest folder still there and forgets the gone ones")
{
	TestTree tree;
	FolderNavigator navigator;
	navigator.Reset({ tree.Root.Get() }, {});
	navigator.Navigate(tree.Get("B"));
	navigator.Navigate(tree.Get("A"));
	navigator.Navigate(tree.Get("A/A1"));

	std::filesystem::remove_all(tree.Get("A").GetAbsolutePath());
	navigator.Refresh();

	CHECK(navigator.GetFolder() == tree.Root.Get());
	// A is gone from the history, so back goes straight to B, then to the root
	navigator.GoBack();
	CHECK(navigator.GetFolder() == tree.Get("B"));
	navigator.GoBack();
	CHECK(navigator.GetFolder() == tree.Root.Get());
	CHECK_FALSE(navigator.CanGoBack());
}

TEST_CASE("A renamed folder takes the folder shown and the history with it, and a deleted one is forgotten")
{
	TestTree tree;
	FolderNavigator navigator;
	navigator.Reset({ tree.Root.Get() }, {});
	navigator.Navigate(tree.Get("A"));
	navigator.Navigate(tree.Get("A/A1"));

	std::filesystem::rename(tree.Get("A").GetAbsolutePath(), tree.Get("Renamed").GetAbsolutePath());
	navigator.OnPathChanged(tree.Get("A"), tree.Get("Renamed"));
	CHECK(navigator.GetFolder() == tree.Get("Renamed/A1"));
	navigator.GoBack();
	CHECK(navigator.GetFolder() == tree.Get("Renamed"));

	std::filesystem::remove_all(tree.Get("Renamed").GetAbsolutePath());
	navigator.OnPathChanged(tree.Get("Renamed"), Filepath());
	CHECK(navigator.GetFolder() == tree.Root.Get());
	CHECK_FALSE(navigator.CanGoForward());
	navigator.GoBack();
	CHECK(navigator.GetFolder() == tree.Root.Get());
}

TEST_CASE("The folder-shown callback runs once each time the folder shown changes")
{
	TestTree tree;
	FolderNavigator navigator;
	int shown = 0;
	navigator.SetOnFolderShown([&shown]() { ++shown; });

	navigator.Reset({ tree.Root.Get() }, {});
	CHECK(shown == 1);
	navigator.Navigate(tree.Get("A"));
	CHECK(shown == 2);
	navigator.Navigate(tree.Get("A"));
	CHECK(shown == 2);
	navigator.GoBack();
	CHECK(shown == 3);
	// Reading the same folder again isn't a change
	navigator.Refresh();
	CHECK(shown == 3);
}

TEST_CASE("FollowRename moves a path at or inside the renamed one, and leaves others")
{
	const Filepath from("Assets/Old");
	const Filepath to("Assets/New");
	CHECK(FollowRename(Filepath("Assets/Old"), from, to) == Filepath("Assets/New"));
	CHECK(FollowRename(Filepath("Assets/Old/Sub/File.png"), from, to) == Filepath("Assets/New/Sub/File.png"));
	CHECK(FollowRename(Filepath("Assets/Older"), from, to) == Filepath("Assets/Older"));
	CHECK(FollowRename(Filepath("Assets/Other"), from, to) == Filepath("Assets/Other"));
}
