#include <UI/Panels/ContentBrowserPanel.h>

#include <Commands/AssetFileCommands.h>
#include <Services/AssetFiles.h>
#include <Services/SceneDocument.h>
#include <Services/ThumbnailRenderer.h>
#include <Services/Thumbnails.h>
#include <UI/Framework/EditorContext.h>
#include <UI/Framework/EditorUI.h>
#include <UI/Framework/PanelRegistry.h>
#include <UI/Modals/UnsavedChangesPrompt.h>

#include <Asset/AssetPaths.h>

#include <Utils/Platform/FileNames.h>
#include <Utils/Platform/Folders.h>
#include <Utils/Platform/Shell.h>

#include <ImGui/imgui.h>

#include <algorithm>

namespace
{
	constexpr float k_minTileSize = 64.0f;
	constexpr float k_maxTileSize = 234.0f;
	constexpr float k_searchWidth = 160.0f;
	constexpr float k_sizeSliderWidth = 68.0f;
	// The folder tree's starting width; the splitter beside it resizes it
	constexpr float k_treeWidth = 200.0f;

	// ImGui's numbers for the mouse's side buttons
	constexpr int k_mouseBack = 3;
	constexpr int k_mouseForward = 4;

	constexpr char k_deletePopupID[] = "Delete?";
	constexpr float k_promptButtonWidth = 110.0f;

	// Why path can't be renamed or deleted, or empty if it can (AssetFiles::GetWhyInUse)
	std::string GetWhyInUse(const Editor::SceneDocument& document, const Filepath& path)
	{
		return Editor::AssetFiles::GetWhyInUse(path, document.HasFilePath() ? document.GetFilePath() : Filepath());
	}

	// Hidden from the tree and the tiles, like the editor's trash
	bool IsHidden(const Filepath& path)
	{
		return path.GetFileName().front() == '.';
	}
}

namespace Editor
{
	REGISTER_EDITOR_PANEL(ContentBrowserPanel);

	struct ContentBrowserPanel::DeletePrompt
	{
		Filepath Path;
		// Opened from Draw, since a request can come from inside a menu, whose ID stack the popup mustn't use
		bool ShouldOpen = true;
	};

	ContentBrowserPanel::ContentBrowserPanel() = default;
	ContentBrowserPanel::~ContentBrowserPanel() = default;

	PanelDesc ContentBrowserPanel::Describe()
	{
		PanelDesc desc;
		desc.Name = "Content Browser";
		desc.MenuPath = "View/Windows/Content Browser";
		desc.DefaultDock = EPanelDock::BottomLeft;
		// The folder tree and the tiles scroll on their own
		desc.WindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
		return desc;
	}

	void ContentBrowserPanel::Initialize(EditorContext& context)
	{
		m_navigator.SetOnFolderShown([this]()
			{
				// Only this folder's images are kept
				m_grid.ReleaseThumbnails();
				m_grid.SetSelected(Filepath(), false);
				m_grid.ScrollToTop();
				m_revealFolder = true;
			});

		FileTileGridOptions options;
		options.ThumbnailFor = &Thumbnails::FindFor;
		// Scenes and meshes have theirs rendered; Request ignores anything else
		options.OnThumbnailMissing = [&context](const Filepath& file) { context.GetThumbnails().Request(file); };
		const auto whyInUse = [&context](const Filepath& file) { return GetWhyInUse(context.GetSceneDocument(), file); };
		options.TileActions = {
			{ "Load Scene", [this, &context](const Filepath& file) { LoadScene(context, file); }, &AssetPaths::IsScene },
			{ "Show in Explorer", [](const Filepath& file) { Shell::ShowInFileBrowser(file); } },
			{ "Copy Path", [](const Filepath& file) { ImGui::SetClipboardText(file.GetRelativePath().c_str()); } },
			{ "Refresh Thumbnail", [&context](const Filepath& file) { context.GetThumbnails().Request(file, true); }, &Thumbnails::IsRendered },
			{ "Rename", [this](const Filepath& file) { m_grid.BeginRename(file); }, nullptr, "F2",
				"Anything that refers to it by path, like a scene using a mesh, isn't updated", whyInUse },
			{ "Delete", [this](const Filepath& file) { RequestDelete(file); }, nullptr, "Delete",
				"Undo puts it back; it goes to the Recycle Bin once it drops out of the undo history", whyInUse },
		};
		options.ValidateName = [](const FolderEntry& entry, const std::string& name) { return FileNames::ResolveRename(entry.Path, entry.IsFolder, name).Error; };
		options.Rename = [this, &context](const FolderEntry& entry, const std::string& name) { Rename(context, entry, name); };
		options.DrawBackgroundMenu = [this]()
			{
				if (ImGui::MenuItem("New Folder"))
				{
					m_createFolderRequested = true;
				}
				ImGui::Separator();
				if (ImGui::MenuItem("Show in Explorer"))
				{
					Shell::ShowInFileBrowser(m_navigator.GetFolder());
				}
				if (ImGui::MenuItem("Refresh"))
				{
					m_refreshRequested = true;
				}
			};
		// Onto the inspector's asset fields that take it (PropertyDrawers), and a mesh into the scene view, which adds an
		// object showing it (SceneViewPanel)
		options.CanDrag = [](const Filepath&) { return true; };
		m_grid.SetOptions(std::move(options));

		m_thumbnailWritten = context.GetEvents().Subscribe<ThumbnailWrittenEvent>([this](const ThumbnailWrittenEvent& event)
			{
				m_grid.RefreshThumbnail(event.Image);
			});
		m_assetFileChanged = context.GetEvents().Subscribe<AssetFileChangedEvent>([this](const AssetFileChangedEvent& event)
			{
				OnAssetFileChanged(event);
			});
		m_projectFilesChanged = context.GetEvents().Subscribe<ProjectFilesChangedEvent>([this](const ProjectFilesChangedEvent&)
			{
				// Read when it next draws, so not while it's hidden
				m_refreshRequested = true;
			});

		// Here rather than at static initialization, so they resolve against the project folder
		m_navigator.Reset({ Filepath("Assets"), Filepath("ProjectData") }, {});
	}

	void ContentBrowserPanel::ShutDown(EditorContext& context)
	{
		m_grid.ReleaseThumbnails();
	}

	void ContentBrowserPanel::Draw(EditorContext& context)
	{
		HandleNavigationInput(context);

		// Made by now, or it couldn't be
		m_renameWhenCreated = Filepath();
		if (m_createFolderRequested)
		{
			m_createFolderRequested = false;
			CreateFolder(context);
		}

		if (m_refreshRequested)
		{
			Refresh();
			RequestOutdatedThumbnails(context);
		}

		DrawToolbar();

		if (ImGui::BeginTable("##layout", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV))
		{
			ImGui::TableSetupColumn("Folders", ImGuiTableColumnFlags_WidthFixed, k_treeWidth);
			ImGui::TableSetupColumn("Files", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableNextRow();

			ImGui::TableNextColumn();
			DrawFolderTree();

			ImGui::TableNextColumn();
			DrawTiles(context);

			ImGui::EndTable();
		}

		DrawDeletePrompt(context);
	}

	void ContentBrowserPanel::DrawToolbar()
	{
		const ImGuiStyle& style = ImGui::GetStyle();
		const float right = ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x;

		DrawFolderNavigationButtons(m_navigator, m_grid);

		ImGui::SameLine();
		const Filepath navigateTo = DrawBreadcrumb(m_navigator.GetRoot(), m_navigator.GetFolder());

		// Tile size, search and refresh, on the right
		ImGui::SameLine(right - k_sizeSliderWidth - k_searchWidth - EditorUI::GetIconButtonWidth() - style.ItemSpacing.x * 2.0f);
		ImGui::SetNextItemWidth(k_sizeSliderWidth);
		ImGui::SliderFloat("##tileSize", &m_tileSize, k_minTileSize, k_maxTileSize, "");
		EditorUI::ItemTooltip("Tile size");

		ImGui::SameLine();
		ImGui::SetNextItemWidth(k_searchWidth);
		if (ImGui::InputTextWithHint("##search", "Search", m_navigator.GetSearchBuffer(), m_navigator.GetSearchBufferSize()))
		{
			m_refreshRequested = true;
			m_grid.ScrollToTop();
		}

		ImGui::SameLine();
		if (EditorUI::IconButton("##refresh", EditorUI::EIcon::Reset, "Refresh"))
		{
			m_refreshRequested = true;
		}

		// After the toolbar, so it isn't drawn for one folder and acted on for another
		if (navigateTo.IsValid())
		{
			m_navigator.Navigate(navigateTo);
		}
	}

	void ContentBrowserPanel::DrawFolderTree()
	{
		if (ImGui::BeginChild("##folders", ImVec2(0.0f, 0.0f)))
		{
			for (const Filepath& root : m_navigator.GetRoots())
			{
				DrawFolderNode(root);
			}
			m_revealFolder = false;
		}
		ImGui::EndChild();
	}

	void ContentBrowserPanel::DrawFolderNode(const Filepath& folder)
	{
		const std::vector<Filepath>& subfolders = GetSubfolders(folder);
		const bool isCurrent = folder == m_navigator.GetFolder();

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
		if (isCurrent)
		{
			flags |= ImGuiTreeNodeFlags_Selected;
		}
		if (subfolders.empty())
		{
			flags |= ImGuiTreeNodeFlags_Leaf;
		}
		if (m_navigator.IsRoot(folder))
		{
			flags |= ImGuiTreeNodeFlags_DefaultOpen;
		}

		// Opens each folder above the current one, e.g. after going there from the tiles
		if (m_revealFolder && !isCurrent && m_navigator.GetFolder().IsInside(folder))
		{
			ImGui::SetNextItemOpen(true);
		}

		const bool isOpen = EditorUI::TreeNode(folder.GetRelativePath().c_str(), flags, "%s", folder.GetFileName().c_str());
		if (m_revealFolder && isCurrent)
		{
			ImGui::SetScrollHereY(0.5f);
		}
		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen() && !isCurrent)
		{
			m_navigator.Navigate(folder);
		}

		if (isOpen)
		{
			// Subfolders stay cached while the tree draws: only Refresh clears them, and that runs before drawing
			for (const Filepath& subfolder : subfolders)
			{
				DrawFolderNode(subfolder);
			}
			ImGui::TreePop();
		}
	}

	void ContentBrowserPanel::DrawTiles(EditorContext& context)
	{
		const char* const emptyText = m_navigator.HasSearch() ? "Nothing here matches the search" : "This folder is empty";
		const FileTileGridResult result = m_grid.Draw(m_navigator.GetEntries(), m_tileSize, ImVec2(0.0f, 0.0f), emptyText);

		if (result.Activated.IsValid())
		{
			Open(context, result.Activated);
		}
	}

	void ContentBrowserPanel::HandleNavigationInput(EditorContext& context)
	{
		// The side buttons work wherever the mouse is over the panel, the keys while it's focused
		if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows))
		{
			if (ImGui::IsMouseClicked(k_mouseBack))
			{
				m_navigator.GoBack();
			}
			else if (ImGui::IsMouseClicked(k_mouseForward))
			{
				m_navigator.GoForward();
			}
		}

		// Typing in the search box keeps its keys
		if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) || ImGui::IsAnyItemActive())
		{
			return;
		}

		if (ImGui::IsKeyPressed(ImGuiKey_Backspace, false))
		{
			GoUpAndSelect(m_navigator, m_grid);
		}
		else if ((ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false)) && m_grid.GetSelected().IsValid())
		{
			Open(context, m_grid.GetSelected());
		}
		else if (ImGui::IsKeyPressed(ImGuiKey_F2, false) || ImGui::IsKeyPressed(ImGuiKey_Delete, false))
		{
			// What the tile's menu offers, so the same files are in use
			const Filepath& selected = m_grid.GetSelected();
			if (m_navigator.FindEntry(selected) == nullptr || !GetWhyInUse(context.GetSceneDocument(), selected).empty())
			{
				return;
			}

			if (ImGui::IsKeyPressed(ImGuiKey_F2, false))
			{
				m_grid.BeginRename(selected);
			}
			else
			{
				RequestDelete(selected);
			}
		}
	}

	void ContentBrowserPanel::DrawDeletePrompt(EditorContext& context)
	{
		if (m_deletePrompt == nullptr)
		{
			return;
		}

		if (m_deletePrompt->ShouldOpen)
		{
			ImGui::OpenPopup(k_deletePopupID);
			m_deletePrompt->ShouldOpen = false;
		}

		const ImGuiViewport* const viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

		const ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
		if (!ImGui::BeginPopupModal(k_deletePopupID, nullptr, flags))
		{
			// Closed some other way
			m_deletePrompt.reset();
			return;
		}

		const Filepath path = m_deletePrompt->Path;
		const FolderEntry* const entry = m_navigator.FindEntry(path);
		ImGui::Text("Delete \"%s\"?", path.GetFileName().c_str());
		if (entry != nullptr && entry->IsFolder)
		{
			ImGui::TextUnformatted("Everything in it goes too.");
		}
		ImGui::TextDisabled("Undo (Ctrl+Z) puts it back.");
		ImGui::TextDisabled("Anything that refers to it by path, like a scene using a mesh, isn't updated.");
		ImGui::Spacing();

		const bool isConfirmed = EditorUI::PrimaryButton("Delete", ImVec2(k_promptButtonWidth, 0.0f))
			|| ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false);
		ImGui::SameLine();
		const bool isCancelled = ImGui::Button("Cancel", ImVec2(k_promptButtonWidth, 0.0f)) || ImGui::IsKeyPressed(ImGuiKey_Escape, false);

		if (isConfirmed || isCancelled)
		{
			ImGui::CloseCurrentPopup();
			m_deletePrompt.reset();
		}
		if (isConfirmed)
		{
			Delete(context, path);
		}

		ImGui::EndPopup();
	}

	void ContentBrowserPanel::Refresh()
	{
		m_refreshRequested = false;
		m_subfolders.clear();
		m_navigator.Refresh();
	}

	void ContentBrowserPanel::RequestOutdatedThumbnails(EditorContext& context)
	{
		for (const FolderEntry& entry : m_navigator.GetEntries())
		{
			// Missing ones are asked for as their tiles show (OnThumbnailMissing). Request skips those up to date.
			if (!entry.IsFolder && Thumbnails::IsRendered(entry.Path) && Thumbnails::FindFor(entry.Path).Exists())
			{
				context.GetThumbnails().Request(entry.Path);
			}
		}
	}

	void ContentBrowserPanel::Open(EditorContext& context, const Filepath& file)
	{
		const FolderEntry* const entry = m_navigator.FindEntry(file);
		if (entry != nullptr && entry->IsFolder)
		{
			m_navigator.Navigate(file);
		}
		else if (AssetPaths::IsScene(file))
		{
			LoadScene(context, file);
		}
	}

	void ContentBrowserPanel::LoadScene(EditorContext& context, const Filepath& scene)
	{
		SceneDocument& document = context.GetSceneDocument();
		// Nothing that touches the scene while one is streaming in
		if (document.IsLoading())
		{
			return;
		}

		context.GetUnsavedChangesPrompt().Request(context, "loading another scene", [&document, scene]() { document.Load(scene); });
	}

	void ContentBrowserPanel::Rename(EditorContext& context, const FolderEntry& entry, const std::string& name)
	{
		const FileNames::ResolvedName renamed = FileNames::ResolveRename(entry.Path, entry.IsFolder, name);
		if (renamed.Path.IsValid() && renamed.Path != entry.Path)
		{
			context.GetCommands().Push<RenameAssetCommand>(entry.Path, renamed.Path);
		}
	}

	void ContentBrowserPanel::RequestDelete(const Filepath& path)
	{
		m_deletePrompt = std::make_unique<DeletePrompt>();
		m_deletePrompt->Path = path;
	}

	void ContentBrowserPanel::Delete(EditorContext& context, const Filepath& path)
	{
		context.GetCommands().Push<DeleteAssetCommand>(path);
	}

	void ContentBrowserPanel::CreateFolder(EditorContext& context)
	{
		m_renameWhenCreated = FileNames::MakeUnique(m_navigator.GetFolder(), "New Folder");
		context.GetCommands().Push<CreateFolderCommand>(m_renameWhenCreated);
	}

	void ContentBrowserPanel::OnAssetFileChanged(const AssetFileChangedEvent& event)
	{
		if (event.From.IsValid())
		{
			m_navigator.OnPathChanged(event.From, event.To);
		}

		const bool isNewFolder = event.To.IsValid() && event.To == m_renameWhenCreated;
		if (isNewFolder)
		{
			// Listed before its rename starts, which ends if it isn't, so a search mustn't hide it
			m_navigator.ClearSearch();
		}
		Refresh();

		if (event.To.IsValid() && event.To.GetParentFolder() == m_navigator.GetFolder())
		{
			m_grid.SetSelected(event.To, true);
		}
		else if (event.From.IsValid() && m_grid.GetSelected() == event.From)
		{
			m_grid.SetSelected(Filepath(), false);
		}

		if (isNewFolder)
		{
			m_grid.BeginRename(event.To);
			m_renameWhenCreated = Filepath();
		}
	}

	const std::vector<Filepath>& ContentBrowserPanel::GetSubfolders(const Filepath& folder)
	{
		auto it = m_subfolders.find(folder.GetRelativePath());
		if (it == m_subfolders.end())
		{
			std::vector<Filepath> subfolders = Folders::ListSubfolders(folder);
			subfolders.erase(std::remove_if(subfolders.begin(), subfolders.end(), &IsHidden), subfolders.end());
			it = m_subfolders.emplace(folder.GetRelativePath(), std::move(subfolders)).first;
		}
		return it->second;
	}
}
