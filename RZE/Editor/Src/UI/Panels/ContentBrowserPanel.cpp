#include <UI/Panels/ContentBrowserPanel.h>

#include <Services/AssetFiles.h>
#include <Services/SceneDocument.h>
#include <Services/ThumbnailRenderer.h>
#include <Services/Thumbnails.h>
#include <UI/Framework/EditorContext.h>
#include <UI/Framework/EditorUI.h>
#include <UI/Framework/PanelRegistry.h>
#include <UI/Modals/UnsavedChangesPrompt.h>

#include <Asset/AssetPaths.h>

#include <DebugUtils/DebugServices.h>

#include <Utils/Platform/Shell.h>
#include <Utils/StringUtils.h>

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

	// The folder is read again this often while the panel is shown, so files added or removed outside it show up
	constexpr std::chrono::seconds k_refreshInterval(2);

	// ImGui's numbers for the mouse's side buttons
	constexpr int k_mouseBack = 3;
	constexpr int k_mouseForward = 4;

	constexpr char k_deletePopupID[] = "Delete?";
	constexpr float k_promptButtonWidth = 110.0f;

	bool IsSameFolder(const Filepath& a, const Filepath& b)
	{
		return a.GetRelativePath() == b.GetRelativePath();
	}

	// Why path can't be renamed or deleted, or empty if it can: the open scene is it, or is inside it
	std::string GetWhyInUse(const Editor::SceneDocument& document, const Filepath& path)
	{
		if (!document.HasFilePath())
		{
			return std::string();
		}

		const Filepath& scene = document.GetFilePath();
		if (StringUtils::EqualsIgnoringCase(scene.GetRelativePath(), path.GetRelativePath()))
		{
			return "It's the open scene";
		}
		return scene.IsInside(path) ? "The open scene is in it" : std::string();
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
		desc.DefaultDock = EPanelDock::Bottom;
		// The folder tree and the tiles scroll on their own
		desc.WindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
		return desc;
	}

	void ContentBrowserPanel::Initialize(EditorContext& context)
	{
		// Here rather than at static initialization, so they resolve against the project folder
		m_roots = { Filepath("Assets"), Filepath("ProjectData") };

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
			{ "Delete", [this](const Filepath& file) { RequestDelete(file); }, nullptr, "Delete", "Moves it to the Recycle Bin", whyInUse },
		};
		options.ValidateName = [](const FolderEntry& entry, const std::string& name) { return ResolveRename(entry.Path, entry.IsFolder, name).Error; };
		options.Rename = [this](const FolderEntry& entry, const std::string& name) { Rename(entry, name); };
		options.DrawBackgroundMenu = [this]()
			{
				if (ImGui::MenuItem("New Folder"))
				{
					m_createFolderRequested = true;
				}
				ImGui::Separator();
				if (ImGui::MenuItem("Show in Explorer"))
				{
					Shell::ShowInFileBrowser(m_folder);
				}
				if (ImGui::MenuItem("Refresh"))
				{
					m_refreshRequested = true;
				}
			};
		// Into the scene view, which adds an object showing it (SceneViewPanel)
		options.CanDrag = &AssetPaths::IsMesh;
		m_grid.SetOptions(std::move(options));

		m_thumbnailWritten = context.GetEvents().Subscribe<ThumbnailWrittenEvent>([this](const ThumbnailWrittenEvent& event)
			{
				m_grid.RefreshThumbnail(event.Image);
			});

		ShowFolder(m_roots.front());
	}

	void ContentBrowserPanel::Draw(EditorContext& context)
	{
		HandleNavigationInput(context);

		if (m_createFolderRequested)
		{
			m_createFolderRequested = false;
			CreateFolder();
		}

		if (m_refreshRequested || std::chrono::steady_clock::now() - m_lastRefresh >= k_refreshInterval)
		{
			Refresh();
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

		DrawDeletePrompt();
	}

	void ContentBrowserPanel::DrawToolbar()
	{
		const ImGuiStyle& style = ImGui::GetStyle();
		const float right = ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x;

		{
			EditorUI::ScopedToolbarSpacing spacing;

			ImGui::BeginDisabled(m_backHistory.empty());
			if (EditorUI::IconButton("##back", EditorUI::EIcon::Back, "Back (mouse 4)"))
			{
				GoBack();
			}
			ImGui::EndDisabled();

			ImGui::SameLine();
			ImGui::BeginDisabled(m_forwardHistory.empty());
			if (EditorUI::IconButton("##forward", EditorUI::EIcon::Forward, "Forward (mouse 5)"))
			{
				GoForward();
			}
			ImGui::EndDisabled();

			ImGui::SameLine();
			ImGui::BeginDisabled(IsRoot(m_folder));
			if (EditorUI::IconButton("##up", EditorUI::EIcon::Up, "Up a folder (Backspace)"))
			{
				GoUp();
			}
			ImGui::EndDisabled();
		}

		ImGui::SameLine();
		const Filepath navigateTo = DrawBreadcrumb(GetRoot(), m_folder);

		// Tile size, search and refresh, on the right
		ImGui::SameLine(right - k_sizeSliderWidth - k_searchWidth - EditorUI::GetIconButtonWidth() - style.ItemSpacing.x * 2.0f);
		ImGui::SetNextItemWidth(k_sizeSliderWidth);
		ImGui::SliderFloat("##tileSize", &m_tileSize, k_minTileSize, k_maxTileSize, "");
		EditorUI::ItemTooltip("Tile size");

		ImGui::SameLine();
		ImGui::SetNextItemWidth(k_searchWidth);
		if (ImGui::InputTextWithHint("##search", "Search", m_search, sizeof(m_search)))
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
			Navigate(navigateTo);
		}
	}

	void ContentBrowserPanel::DrawFolderTree()
	{
		if (ImGui::BeginChild("##folders", ImVec2(0.0f, 0.0f)))
		{
			for (const Filepath& root : m_roots)
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
		const bool isCurrent = IsSameFolder(folder, m_folder);

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
		if (isCurrent)
		{
			flags |= ImGuiTreeNodeFlags_Selected;
		}
		if (subfolders.empty())
		{
			flags |= ImGuiTreeNodeFlags_Leaf;
		}
		if (IsRoot(folder))
		{
			flags |= ImGuiTreeNodeFlags_DefaultOpen;
		}

		// Opens each folder above the current one, e.g. after going there from the tiles
		if (m_revealFolder && !isCurrent && m_folder.IsInside(folder))
		{
			ImGui::SetNextItemOpen(true);
		}

		const bool isOpen = ImGui::TreeNodeEx(folder.GetRelativePath().c_str(), flags, "%s", folder.GetFileName().c_str());
		if (m_revealFolder && isCurrent)
		{
			ImGui::SetScrollHereY(0.5f);
		}
		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen() && !isCurrent)
		{
			Navigate(folder);
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
		const char* const emptyText = m_search[0] != '\0' ? "Nothing here matches the search" : "This folder is empty";
		const FileTileGridResult result = m_grid.Draw(m_entries, m_tileSize, ImVec2(0.0f, 0.0f), emptyText);

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
				GoBack();
			}
			else if (ImGui::IsMouseClicked(k_mouseForward))
			{
				GoForward();
			}
		}

		// Typing in the search box keeps its keys
		if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) || ImGui::IsAnyItemActive())
		{
			return;
		}

		if (ImGui::IsKeyPressed(ImGuiKey_Backspace, false))
		{
			GoUp();
		}
		else if ((ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false)) && m_grid.GetSelected().IsValid())
		{
			Open(context, m_grid.GetSelected());
		}
		else if (ImGui::IsKeyPressed(ImGuiKey_F2, false) || ImGui::IsKeyPressed(ImGuiKey_Delete, false))
		{
			// What the tile's menu offers, so the same files are in use
			const Filepath& selected = m_grid.GetSelected();
			if (FindEntry(selected) == nullptr || !GetWhyInUse(context.GetSceneDocument(), selected).empty())
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

	void ContentBrowserPanel::DrawDeletePrompt()
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
		const FolderEntry* const entry = FindEntry(path);
		ImGui::Text("Move \"%s\" to the Recycle Bin?", path.GetFileName().c_str());
		if (entry != nullptr && entry->IsFolder)
		{
			ImGui::TextUnformatted("Everything in it goes too.");
		}
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
			Delete(path);
		}

		ImGui::EndPopup();
	}

	void ContentBrowserPanel::Navigate(const Filepath& folder)
	{
		if (IsSameFolder(folder, m_folder))
		{
			return;
		}

		m_backHistory.push_back(m_folder);
		m_forwardHistory.clear();
		ShowFolder(folder);
	}

	void ContentBrowserPanel::GoBack()
	{
		if (m_backHistory.empty())
		{
			return;
		}

		m_forwardHistory.push_back(m_folder);
		const Filepath folder = m_backHistory.back();
		m_backHistory.pop_back();
		ShowFolder(folder);
	}

	void ContentBrowserPanel::GoForward()
	{
		if (m_forwardHistory.empty())
		{
			return;
		}

		m_backHistory.push_back(m_folder);
		const Filepath folder = m_forwardHistory.back();
		m_forwardHistory.pop_back();
		ShowFolder(folder);
	}

	void ContentBrowserPanel::GoUp()
	{
		if (IsRoot(m_folder))
		{
			return;
		}

		// Selects the folder it came out of, so it's easy to see where it was
		const Filepath child = m_folder;
		Navigate(m_folder.GetParentFolder());
		m_grid.SetSelected(child, true);
	}

	void ContentBrowserPanel::ShowFolder(const Filepath& folder)
	{
		m_folder = folder;
		m_search[0] = '\0';
		// Only this folder's images are kept
		m_grid.ReleaseThumbnails();
		m_grid.SetSelected(Filepath(), false);
		m_grid.ScrollToTop();
		m_revealFolder = true;

		m_entries = ListFolder(m_folder, {}, m_search);
	}

	void ContentBrowserPanel::Refresh()
	{
		m_entries = ListFolder(m_folder, {}, m_search);
		m_subfolders.clear();
		m_lastRefresh = std::chrono::steady_clock::now();
		m_refreshRequested = false;
	}

	void ContentBrowserPanel::Open(EditorContext& context, const Filepath& file)
	{
		const FolderEntry* const entry = FindEntry(file);
		if (entry != nullptr && entry->IsFolder)
		{
			Navigate(file);
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

	void ContentBrowserPanel::Rename(const FolderEntry& entry, const std::string& name)
	{
		const ResolvedName renamed = ResolveRename(entry.Path, entry.IsFolder, name);
		if (!renamed.Path.IsValid() || renamed.Path.GetRelativePath() == entry.Path.GetRelativePath())
		{
			return;
		}

		if (!AssetFiles::Rename(entry.Path, renamed.Path))
		{
			DebugServices::Get().Trace(LogChannel::Info, "Couldn't rename " + entry.Path.GetRelativePath() + ". Is it open in another program?");
			return;
		}

		MoveHistory(entry.Path, renamed.Path);
		Refresh();
		m_grid.SetSelected(renamed.Path, true);
	}

	void ContentBrowserPanel::RequestDelete(const Filepath& path)
	{
		m_deletePrompt = std::make_unique<DeletePrompt>();
		m_deletePrompt->Path = path;
	}

	void ContentBrowserPanel::Delete(const Filepath& path)
	{
		if (!AssetFiles::Delete(path))
		{
			DebugServices::Get().Trace(LogChannel::Info, "Couldn't delete " + path.GetRelativePath() + ". Is it open in another program?");
			return;
		}

		ForgetHistory(path);
		m_grid.SetSelected(Filepath(), false);
		Refresh();
	}

	void ContentBrowserPanel::CreateFolder()
	{
		const Filepath folder = AssetFiles::CreateFolder(m_folder, "New Folder");
		if (!folder.IsValid())
		{
			DebugServices::Get().Trace(LogChannel::Info, "Couldn't make a folder in " + m_folder.GetRelativePath());
			return;
		}

		// Listed before its rename starts, which ends if it isn't, so a search mustn't hide it
		m_search[0] = '\0';
		Refresh();
		m_grid.BeginRename(folder);
	}

	void ContentBrowserPanel::MoveHistory(const Filepath& from, const Filepath& to)
	{
		const size_t fromLength = from.GetRelativePath().size();
		const auto move = [&from, &to, fromLength](Filepath& folder)
			{
				if (IsSameFolder(folder, from) || folder.IsInside(from))
				{
					folder = Filepath(to.GetRelativePath() + folder.GetRelativePath().substr(fromLength));
				}
			};

		std::for_each(m_backHistory.begin(), m_backHistory.end(), move);
		std::for_each(m_forwardHistory.begin(), m_forwardHistory.end(), move);
	}

	void ContentBrowserPanel::ForgetHistory(const Filepath& path)
	{
		const auto isGone = [&path](const Filepath& folder) { return IsSameFolder(folder, path) || folder.IsInside(path); };
		m_backHistory.erase(std::remove_if(m_backHistory.begin(), m_backHistory.end(), isGone), m_backHistory.end());
		m_forwardHistory.erase(std::remove_if(m_forwardHistory.begin(), m_forwardHistory.end(), isGone), m_forwardHistory.end());
	}

	const FolderEntry* ContentBrowserPanel::FindEntry(const Filepath& path) const
	{
		const auto entry = std::find_if(m_entries.begin(), m_entries.end(),
			[&path](const FolderEntry& candidate) { return candidate.Path.GetRelativePath() == path.GetRelativePath(); });
		return entry != m_entries.end() ? &*entry : nullptr;
	}

	const Filepath& ContentBrowserPanel::GetRoot() const
	{
		const auto root = std::find_if(m_roots.begin(), m_roots.end(),
			[this](const Filepath& candidate) { return IsSameFolder(m_folder, candidate) || m_folder.IsInside(candidate); });
		return root != m_roots.end() ? *root : m_roots.front();
	}

	bool ContentBrowserPanel::IsRoot(const Filepath& folder) const
	{
		return std::any_of(m_roots.begin(), m_roots.end(), [&folder](const Filepath& root) { return IsSameFolder(folder, root); });
	}

	const std::vector<Filepath>& ContentBrowserPanel::GetSubfolders(const Filepath& folder)
	{
		auto it = m_subfolders.find(folder.GetRelativePath());
		if (it == m_subfolders.end())
		{
			it = m_subfolders.emplace(folder.GetRelativePath(), ListSubfolders(folder)).first;
		}
		return it->second;
	}
}
