#include <doctest/doctest.h>

#include <Commands/AssetFileCommands.h>
#include <Services/AssetFiles.h>
#include <Services/UndoHistory.h>
#include <UI/Framework/EditorContext.h>

#include <TestFolder.h>

#include <filesystem>
#include <memory>
#include <vector>

using namespace Editor;

namespace
{
	// Runs commands the way CommandQueue::Flush does, recording each one's undo step, and keeps the AssetFileChangedEvents
	struct AssetFileCommandsFixture
	{
		EditorContext Context;
		UndoHistory History;
		ScopedTestFolder Folder{ "UnitTestsTemp_AssetFileCommands" };

		struct Change
		{
			std::string From;
			std::string To;
		};
		std::vector<Change> Changes;
		EventSubscription Changed = Context.GetEvents().Subscribe<AssetFileChangedEvent>([this](const AssetFileChangedEvent& event)
			{
				Changes.push_back({ event.From.IsValid() ? event.From.GetFileName() : "", event.To.IsValid() ? event.To.GetFileName() : "" });
			});

		~AssetFileCommandsFixture()
		{
			// Like closing the editor, but without the Recycle Bin. Tests resolve paths beside UnitTests.exe, so this is
			// the tests' own trash, not the editor's.
			std::error_code ignored;
			std::filesystem::remove_all(AssetFiles::GetTrashFolder().GetAbsolutePath(), ignored);
		}

		bool Run(EditorCommand& command)
		{
			if (!command.Execute(Context))
			{
				return false;
			}
			History.Record(command.GetInteraction(), command.TakeUndoRecord());
			return true;
		}
	};
}

TEST_CASE_FIXTURE(AssetFileCommandsFixture, "Renaming a file can be undone and redone, without touching the save point")
{
	const Filepath tank = Folder.AddFile("Tank.meshasset");
	const Filepath panzer = Folder.GetFile("Panzer.meshasset");
	RenameAssetCommand rename(tank, panzer);

	REQUIRE(Run(rename));
	CHECK(panzer.Exists());
	CHECK(History.GetUndoName() == "Rename File");
	CHECK(History.IsAtSavePoint());

	History.Undo(Context);
	CHECK(tank.Exists());
	CHECK_FALSE(panzer.Exists());

	History.Redo(Context);
	CHECK(panzer.Exists());

	REQUIRE(Changes.size() == 3);
	CHECK(Changes[1].From == "Panzer.meshasset");
	CHECK(Changes[1].To == "Tank.meshasset");
}

TEST_CASE_FIXTURE(AssetFileCommandsFixture, "Undoing a rename leaves it if something else has the old name now")
{
	const Filepath tank = Folder.AddFile("Tank.meshasset");
	const Filepath panzer = Folder.GetFile("Panzer.meshasset");
	RenameAssetCommand rename(tank, panzer);
	REQUIRE(Run(rename));
	Folder.AddFile("Tank.meshasset");

	History.Undo(Context);

	CHECK(panzer.Exists());
	CHECK(Changes.size() == 1);
	// Redo has nothing to do, since it's still renamed
	History.Redo(Context);
	CHECK(panzer.Exists());
	CHECK(Changes.size() == 1);
}

TEST_CASE_FIXTURE(AssetFileCommandsFixture, "Renaming something that isn't there changes nothing")
{
	RenameAssetCommand missing(Folder.GetFile("Missing.meshasset"), Folder.GetFile("Other.meshasset"));
	CHECK_FALSE(Run(missing));
	CHECK_FALSE(History.CanUndo());
}

TEST_CASE_FIXTURE(AssetFileCommandsFixture, "Deleting a folder can be undone, even after the scene's steps are cleared")
{
	const Filepath tank = Folder.AddFile("Vehicles/Tank.meshasset");
	const Filepath vehicles = tank.GetParentFolder();
	DeleteAssetCommand remove(vehicles);

	REQUIRE(Run(remove));
	CHECK_FALSE(vehicles.Exists());
	CHECK(History.GetUndoName() == "Delete Folder");

	// Loading another scene
	History.Clear();
	REQUIRE(History.CanUndo());
	History.Undo(Context);
	CHECK(tank.Exists());

	History.Redo(Context);
	CHECK_FALSE(vehicles.Exists());
	History.Undo(Context);
	CHECK(tank.Exists());
}

TEST_CASE_FIXTURE(AssetFileCommandsFixture, "Undoing a delete leaves it in the trash if something else has its name now")
{
	const Filepath tank = Folder.AddFile("Tank.meshasset");
	DeleteAssetCommand remove(tank);
	REQUIRE(Run(remove));
	Folder.AddFile("Tank.meshasset");

	History.Undo(Context);

	CHECK(Changes.size() == 1);
	CHECK(std::filesystem::exists(AssetFiles::GetTrashFolder().GetAbsolutePath()));
}

TEST_CASE_FIXTURE(AssetFileCommandsFixture, "Undoing a new folder removes it only while it's empty")
{
	const Filepath newFolder = Folder.GetFile("New Folder");
	CreateFolderCommand create(newFolder);

	REQUIRE(Run(create));
	CHECK(std::filesystem::is_directory(newFolder.GetAbsolutePath()));
	CHECK(History.GetUndoName() == "New Folder");

	History.Undo(Context);
	CHECK_FALSE(newFolder.Exists());
	History.Redo(Context);
	CHECK(newFolder.Exists());

	Folder.AddFile("New Folder/Notes.txt");
	History.Undo(Context);
	CHECK(newFolder.Exists());
}

TEST_CASE_FIXTURE(AssetFileCommandsFixture, "Making a folder that's already there changes nothing")
{
	Folder.AddFolder("New Folder");
	CreateFolderCommand create(Folder.GetFile("New Folder"));

	CHECK_FALSE(Run(create));
	CHECK(Changes.empty());
}
