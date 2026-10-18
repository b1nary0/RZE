#include <Commands/AssetFileCommands.h>

#include <Services/AssetFiles.h>
#include <Services/SceneDocument.h>
#include <UI/Framework/EditorContext.h>

#include <DebugUtils/DebugServices.h>

#include <Utils/StringUtils.h>

#include <filesystem>

namespace
{
	using Editor::EditorContext;

	bool IsFolder(const Filepath& path)
	{
		std::error_code error;
		return std::filesystem::is_directory(path.GetAbsolutePath(), error);
	}

	Filepath GetOpenScene(EditorContext& context)
	{
		const Editor::SceneDocument& document = context.GetSceneDocument();
		return document.HasFilePath() ? document.GetFilePath() : Filepath();
	}

	// e.g. "Couldn't undo deleting Assets/Tank.meshasset. Something else is called that now."
	void ReportFailure(const std::string& action, const Filepath& path, const std::string& reason)
	{
		DebugServices::Get().Trace(LogChannel::Info, "Couldn't " + action + " " + path.GetRelativePath() + ". " + reason + ".");
	}

	void PublishChange(EditorContext& context, const Filepath& from, const Filepath& to)
	{
		context.GetEvents().Publish(Editor::AssetFileChangedEvent{ from, to });
	}

	// Renames from to to unless that can't be done any more, for the command and its undo and redo. action is what the
	// Log says couldn't be done, e.g. "undo renaming".
	bool TryRename(EditorContext& context, const Filepath& from, const Filepath& to, const std::string& action)
	{
		std::string whyNot;
		if (!from.Exists())
		{
			whyNot = "It isn't there any more";
		}
		// Changing only the case is fine: on Windows that's the same name
		else if (to.Exists() && !StringUtils::EqualsIgnoringCase(from.GetRelativePath(), to.GetRelativePath()))
		{
			whyNot = "Something else is called " + to.GetFileName() + " now";
		}
		else
		{
			whyNot = Editor::AssetFiles::GetWhyInUse(from, GetOpenScene(context));
		}

		if (whyNot.empty() && !Editor::AssetFiles::Rename(from, to))
		{
			whyNot = "It may be open in another program";
		}

		if (!whyNot.empty())
		{
			ReportFailure(action, from, whyNot);
			return false;
		}

		PublishChange(context, from, to);
		return true;
	}

	// Moves path into the trash unless that can't be done any more; not IsInTrash if it wasn't
	Editor::TrashedItem TryTrash(EditorContext& context, const Filepath& path, const std::string& action)
	{
		std::string whyNot = path.Exists() ? Editor::AssetFiles::GetWhyInUse(path, GetOpenScene(context)) : std::string("It isn't there any more");
		Editor::TrashedItem item;
		item.Original = path;
		if (whyNot.empty())
		{
			item = Editor::AssetFiles::MoveToTrash(path);
			if (!item.IsInTrash())
			{
				whyNot = "It, or something in it, may be open in another program";
			}
		}

		if (!whyNot.empty())
		{
			ReportFailure(action, path, whyNot);
			return item;
		}

		PublishChange(context, path, Filepath());
		return item;
	}

	class RenameAssetRecord : public Editor::UndoRecord
	{
	public:
		RenameAssetRecord(const Filepath& from, const Filepath& to, bool isFolder)
			: m_from(from)
			, m_to(to)
			, m_isFolder(isFolder)
		{
		}

		void Undo(EditorContext& context) override
		{
			if (m_isRenamed && TryRename(context, m_to, m_from, "undo renaming"))
			{
				m_isRenamed = false;
			}
		}

		void Redo(EditorContext& context) override
		{
			if (!m_isRenamed && TryRename(context, m_from, m_to, "redo renaming"))
			{
				m_isRenamed = true;
			}
		}

		std::string GetName() const override { return m_isFolder ? "Rename Folder" : "Rename File"; }
		bool ChangesScene() const override { return false; }

	private:
		Filepath m_from;
		Filepath m_to;
		bool m_isFolder;
		// False once an undo has put it back, so a redo has something to do; an undo that couldn't leaves it true
		bool m_isRenamed = true;
	};

	class DeleteAssetRecord : public Editor::UndoRecord
	{
	public:
		DeleteAssetRecord(const Editor::TrashedItem& item, bool isFolder)
			: m_item(item)
			, m_isFolder(isFolder)
		{
		}

		void Undo(EditorContext& context) override
		{
			if (!m_item.IsInTrash())
			{
				return;
			}

			std::string whyNot;
			if (m_item.Original.Exists())
			{
				whyNot = "Something else is called that now";
			}
			else if (!Editor::AssetFiles::RestoreFromTrash(m_item))
			{
				whyNot = "The folder it was in may have gone, or it's open in another program";
			}

			if (!whyNot.empty())
			{
				ReportFailure("undo deleting", m_item.Original, whyNot);
				return;
			}

			PublishChange(context, Filepath(), m_item.Original);
			m_item.InTrash = Filepath();
			m_item.Thumbnails.clear();
		}

		void Redo(EditorContext& context) override
		{
			if (!m_item.IsInTrash())
			{
				m_item = TryTrash(context, m_item.Original, "redo deleting");
			}
		}

		std::string GetName() const override { return m_isFolder ? "Delete Folder" : "Delete File"; }
		bool ChangesScene() const override { return false; }

		void OnDropped() override
		{
			// Nothing can put it back now. If the Recycle Bin won't take it yet, EmptyTrash tries again.
			if (m_item.IsInTrash())
			{
				Editor::AssetFiles::Recycle(m_item);
			}
		}

	private:
		// In the trash, or just its Original path once an undo has put it back
		Editor::TrashedItem m_item;
		bool m_isFolder;
	};

	class CreateFolderRecord : public Editor::UndoRecord
	{
	public:
		explicit CreateFolderRecord(const Filepath& folder)
			: m_folder(folder)
		{
		}

		void Undo(EditorContext& context) override
		{
			if (!m_exists)
			{
				return;
			}

			std::error_code error;
			if (!IsFolder(m_folder))
			{
				ReportFailure("undo making", m_folder, "It isn't there any more");
				m_exists = false;
			}
			else if (!std::filesystem::is_empty(m_folder.GetAbsolutePath(), error))
			{
				ReportFailure("undo making", m_folder, "Something has been put in it");
			}
			else if (std::filesystem::remove(m_folder.GetAbsolutePath(), error))
			{
				PublishChange(context, m_folder, Filepath());
				m_exists = false;
			}
			else
			{
				ReportFailure("undo making", m_folder, "It may be open in another program");
			}
		}

		void Redo(EditorContext& context) override
		{
			if (m_exists)
			{
				return;
			}

			if (m_folder.Exists())
			{
				ReportFailure("redo making", m_folder, "Something else is called that now");
			}
			else if (Editor::AssetFiles::CreateFolder(m_folder))
			{
				PublishChange(context, Filepath(), m_folder);
				m_exists = true;
			}
			else
			{
				ReportFailure("redo making", m_folder, "The folder it goes in may have gone");
			}
		}

		std::string GetName() const override { return "New Folder"; }
		bool ChangesScene() const override { return false; }

	private:
		Filepath m_folder;
		// False once an undo has removed it
		bool m_exists = true;
	};
}

namespace Editor
{
	RenameAssetCommand::RenameAssetCommand(const Filepath& from, const Filepath& to)
		: m_from(from)
		, m_to(to)
	{
	}

	bool RenameAssetCommand::Execute(EditorContext& context)
	{
		// Read first, since from is gone once it's renamed
		const bool isFolder = IsFolder(m_from);
		if (!TryRename(context, m_from, m_to, "rename"))
		{
			return false;
		}

		SetUndoRecord(std::make_unique<RenameAssetRecord>(m_from, m_to, isFolder));
		return true;
	}

	DeleteAssetCommand::DeleteAssetCommand(const Filepath& path)
		: m_path(path)
	{
	}

	bool DeleteAssetCommand::Execute(EditorContext& context)
	{
		const bool isFolder = IsFolder(m_path);
		const TrashedItem item = TryTrash(context, m_path, "delete");
		if (!item.IsInTrash())
		{
			return false;
		}

		SetUndoRecord(std::make_unique<DeleteAssetRecord>(item, isFolder));
		return true;
	}

	CreateFolderCommand::CreateFolderCommand(const Filepath& folder)
		: m_folder(folder)
	{
	}

	bool CreateFolderCommand::Execute(EditorContext& context)
	{
		if (m_folder.Exists())
		{
			ReportFailure("make", m_folder, "Something is already called that");
			return false;
		}
		if (!AssetFiles::CreateFolder(m_folder))
		{
			ReportFailure("make", m_folder, "The folder it goes in may have gone");
			return false;
		}

		PublishChange(context, Filepath(), m_folder);
		SetUndoRecord(std::make_unique<CreateFolderRecord>(m_folder));
		return true;
	}
}
