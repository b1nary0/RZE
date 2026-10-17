#include <UI/Modals/FileBrowserModal.h>

#include <UI/Framework/EditorUI.h>

#include <algorithm>
#include <cstring>

namespace
{
	constexpr char k_popupID[] = "##FileBrowser";

	constexpr float k_windowPadding = 13.0f;
	constexpr float k_windowRounding = 6.0f;
	constexpr float k_titleFontSize = 15.0f;
	constexpr float k_searchWidth = 160.0f;
	constexpr float k_sizeSliderWidth = 68.0f;
	constexpr float k_buttonWidth = 74.0f;

	constexpr float k_minTileSize = 95.0f;
	constexpr float k_maxTileSize = 234.0f;

	const ImVec4 k_warningColour(1.0f, 0.78f, 0.35f, 1.0f);
	const ImVec4 k_errorColour(1.0f, 0.45f, 0.40f, 1.0f);
}

namespace Editor
{
	void FileBrowserModal::Open(FileBrowserRequest request)
	{
		m_grid.ReleaseThumbnails();

		// A request still open is replaced, so it's answered as cancelled once the new one is set up
		std::function<void()> replacedCancelled = IsOpen() ? std::move(m_request.OnCancelled) : nullptr;

		m_request = std::move(request);
		m_folder = m_request.RootFolder;
		m_grid.SetSelected(Filepath(), false);
		m_grid.SetOptions({ m_request.ThumbnailFor, m_request.OnThumbnailMissing, m_request.TileActions });
		m_search[0] = '\0';

		// Starts in the initial selection's folder if that's inside the root
		if (m_request.InitialSelection.IsValid() && m_request.InitialSelection.GetParentFolder().IsInside(m_request.RootFolder))
		{
			m_folder = m_request.InitialSelection.GetParentFolder();
			m_grid.SetSelected(m_request.InitialSelection, true);
		}
		else
		{
			m_grid.ScrollToTop();
		}

		Refresh();

		m_saveError.clear();
		m_pendingReplace = Filepath();
		if (IsSaving())
		{
			SetFileName(m_request.InitialSelection.IsValid() ? m_request.InitialSelection.GetStem() : m_request.DefaultFileName);
			m_focusFileName = true;
		}

		// Opened from Display, since a request can come from inside a menu or a panel, whose ID stack the popup mustn't use
		m_shouldOpen = true;
		m_centreOnOpen = true;

		if (replacedCancelled)
		{
			replacedCancelled();
		}
	}

	bool FileBrowserModal::ConsumeResult(ImGuiID requesterID, Filepath& outFile)
	{
		if (requesterID == 0 || requesterID != m_resultRequester)
		{
			return false;
		}

		outFile = m_result;
		m_resultRequester = 0;
		m_result = Filepath();
		return true;
	}

	void FileBrowserModal::RefreshThumbnail(const Filepath& image)
	{
		m_grid.RefreshThumbnail(image);
	}

	void FileBrowserModal::Display()
	{
		// Another dialog opened at the root, such as a tile action's unsaved-changes prompt or progress dialog, takes the
		// browser's place on screen (ImGui keeps one popup per level). The browser waits under it and comes back after.
		if (m_isOpen && ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId) && !ImGui::IsPopupOpen(k_popupID))
		{
			m_shouldOpen = true;
			return;
		}

		if (m_shouldOpen)
		{
			ImGui::OpenPopup(k_popupID);
			m_shouldOpen = false;
			m_isOpen = true;
		}

		if (!m_isOpen)
		{
			return;
		}

		const ImGuiViewport* const viewport = ImGui::GetMainViewport();
		const ImVec2 size(std::min(viewport->Size.x, std::max(560.0f, viewport->Size.x * 0.43f)),
			std::min(viewport->Size.y, std::max(380.0f, viewport->Size.y * 0.47f)));
		if (m_centreOnOpen)
		{
			ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
			m_centreOnOpen = false;
		}
		ImGui::SetNextWindowSize(size, ImGuiCond_Always);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(k_windowPadding, k_windowPadding));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, k_windowRounding);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
		ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.11f, 0.11f, 0.12f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 1.0f, 1.0f, 0.10f));
		ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0.0f, 0.0f, 0.0f, 0.55f));

		// Dragging any empty part of the header or footer moves it
		const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings
			| ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
		const bool isVisible = ImGui::BeginPopupModal(k_popupID, nullptr, flags);

		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar(3);

		if (!isVisible)
		{
			// Closed by something other than the browser
			m_isOpen = false;
			m_grid.ReleaseThumbnails();
			std::function<void()> cancelled = std::move(m_request.OnCancelled);
			m_request = FileBrowserRequest();
			if (cancelled)
			{
				cancelled();
			}
			return;
		}

		DrawHeader();
		if (m_isOpen)
		{
			DrawTiles();
		}
		if (m_isOpen)
		{
			DrawFooter();
		}

		ImGui::EndPopup();
	}

	void FileBrowserModal::DrawHeader()
	{
		const ImVec2 start = ImGui::GetCursorPos();
		const float right = start.x + ImGui::GetContentRegionAvail().x;

		ImGui::PushFont(nullptr, k_titleFontSize);
		ImGui::TextUnformatted(m_request.Title.c_str());
		ImGui::PopFont();

		const Filepath navigateTo = DrawBreadcrumb(m_request.RootFolder, m_folder);
		if (navigateTo.IsValid())
		{
			Navigate(navigateTo);
		}

		const float headerBottom = ImGui::GetCursorPosY();

		// Tile size and search, on the right, centred on the title and breadcrumb
		const ImGuiStyle& style = ImGui::GetStyle();
		const float controlsY = start.y + (headerBottom - style.ItemSpacing.y - start.y - ImGui::GetFrameHeight()) * 0.5f;
		ImGui::SetCursorPos(ImVec2(right - k_searchWidth - style.ItemSpacing.x - k_sizeSliderWidth, controlsY));

		ImGui::SetNextItemWidth(k_sizeSliderWidth);
		ImGui::SliderFloat("##tileSize", &m_tileSize, k_minTileSize, k_maxTileSize, "");
		EditorUI::ItemTooltip("Tile size");

		ImGui::SameLine();
		ImGui::SetNextItemWidth(k_searchWidth);
		if (ImGui::InputTextWithHint("##search", "Search", m_search, sizeof(m_search)))
		{
			Refresh();
			m_grid.ScrollToTop();
		}
		m_isTextActive = ImGui::IsItemActive() || ImGui::IsItemDeactivated();

		ImGui::SetCursorPos(ImVec2(start.x, headerBottom));
		ImGui::Spacing();
	}

	void FileBrowserModal::DrawTiles()
	{
		HandleKeys();
		if (!m_isOpen)
		{
			return;
		}

		const ImGuiStyle& style = ImGui::GetStyle();
		const bool hasNoticeRow = IsSaving() && !m_saveError.empty() && !m_pendingReplace.IsValid();
		const float footerHeight = ImGui::GetFrameHeight() + style.ItemSpacing.y * 2.0f + 6.0f
			+ (hasNoticeRow ? ImGui::GetTextLineHeightWithSpacing() : 0.0f);

		const char* const emptyText = m_search[0] != '\0' ? "Nothing here matches the search" : "This folder is empty";
		const FileTileGridResult result = m_grid.Draw(m_entries, m_tileSize, ImVec2(0.0f, -footerHeight), emptyText);

		// Save mode: clicking a file copies its name in
		if (result.Clicked.IsValid() && IsSaving())
		{
			const FolderEntry* const clicked = GetSelected();
			if (clicked != nullptr && !clicked->IsFolder)
			{
				SetFileName(clicked->Path.GetStem());
			}
		}

		if (result.Activated.IsValid())
		{
			m_grid.SetSelected(result.Activated, false);
			ConfirmSelection();
		}
	}

	void FileBrowserModal::DrawFooter()
	{
		ImGui::Spacing();

		const ImGuiStyle& style = ImGui::GetStyle();
		const float buttonsX = ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - k_buttonWidth * 2.0f - style.ItemSpacing.x;
		const FolderEntry* const selected = GetSelected();

		bool isCancelled = false;
		bool isConfirmed = false;

		if (!IsSaving())
		{
			ImGui::AlignTextToFramePadding();
			if (selected != nullptr)
			{
				ImGui::TextDisabled("%s", selected->Path.GetRelativePath().c_str());
			}
			else
			{
				ImGui::TextDisabled("Double-click a file to open it");
			}

			ImGui::SameLine(buttonsX);
			isCancelled = ImGui::Button("Cancel", ImVec2(k_buttonWidth, 0.0f));
			ImGui::SameLine();
			ImGui::BeginDisabled(selected == nullptr);
			isConfirmed = EditorUI::PrimaryButton(m_request.ConfirmLabel.c_str(), ImVec2(k_buttonWidth, 0.0f));
			ImGui::EndDisabled();
		}
		else if (m_pendingReplace.IsValid())
		{
			// Asks in place, rather than in a second popup over this one
			ImGui::AlignTextToFramePadding();
			ImGui::TextColored(k_warningColour, "%s already exists. Replace it?", m_pendingReplace.GetFileName().c_str());

			ImGui::SameLine(buttonsX);
			if (ImGui::Button("Back", ImVec2(k_buttonWidth, 0.0f)))
			{
				m_pendingReplace = Filepath();
			}
			ImGui::SameLine();
			if (EditorUI::PrimaryButton("Replace", ImVec2(k_buttonWidth, 0.0f)))
			{
				Choose(m_pendingReplace);
				return;
			}
		}
		else
		{
			if (!m_saveError.empty())
			{
				ImGui::TextColored(k_errorColour, "%s", m_saveError.c_str());
			}

			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted("File name");
			ImGui::SameLine();

			if (m_focusFileName)
			{
				ImGui::SetKeyboardFocusHere();
				m_focusFileName = false;
			}
			ImGui::SetNextItemWidth(std::max(1.0f, buttonsX - ImGui::GetCursorPosX() - style.ItemSpacing.x));
			const bool isEntered = ImGui::InputText("##fileName", m_fileName, sizeof(m_fileName), ImGuiInputTextFlags_EnterReturnsTrue);
			if (ImGui::IsItemEdited())
			{
				m_saveError.clear();
			}
			m_isTextActive = m_isTextActive || ImGui::IsItemActive() || ImGui::IsItemDeactivated();

			ImGui::SameLine(buttonsX);
			isCancelled = ImGui::Button("Cancel", ImVec2(k_buttonWidth, 0.0f));
			ImGui::SameLine();
			ImGui::BeginDisabled(m_fileName[0] == 0);
			isConfirmed = EditorUI::PrimaryButton(m_request.ConfirmLabel.c_str(), ImVec2(k_buttonWidth, 0.0f)) || isEntered;
			ImGui::EndDisabled();
		}

		if (isCancelled)
		{
			Cancel();
		}
		else if (isConfirmed)
		{
			if (IsSaving())
			{
				ConfirmSave();
			}
			else
			{
				ConfirmSelection();
			}
		}
	}

	void FileBrowserModal::HandleKeys()
	{
		if (ImGui::IsKeyPressed(ImGuiKey_Escape, false) && !m_isTextActive)
		{
			// Backs out of the replace question first
			if (m_pendingReplace.IsValid())
			{
				m_pendingReplace = Filepath();
			}
			else
			{
				Cancel();
			}
			return;
		}

		// Typing in a text field keeps its keys
		if (ImGui::IsAnyItemActive())
		{
			return;
		}

		if (ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false))
		{
			ConfirmSelection();
			return;
		}

		if (ImGui::IsKeyPressed(ImGuiKey_Backspace, false) && m_folder.GetRelativePath() != m_request.RootFolder.GetRelativePath())
		{
			Navigate(m_folder.GetParentFolder());
		}
	}

	void FileBrowserModal::Navigate(const Filepath& folder)
	{
		m_folder = folder;
		m_search[0] = '\0';
		Refresh();
		m_grid.ScrollToTop();
	}

	void FileBrowserModal::Refresh()
	{
		m_entries = ListFolder(m_folder, m_request.Extensions, m_search);
	}

	void FileBrowserModal::ConfirmSelection()
	{
		const FolderEntry* const selected = GetSelected();
		if (selected != nullptr && selected->IsFolder)
		{
			const Filepath folder = selected->Path;
			m_grid.SetSelected(Filepath(), false);
			Navigate(folder);
		}
		else if (IsSaving())
		{
			ConfirmSave();
		}
		else if (selected != nullptr)
		{
			Choose(selected->Path);
		}
	}

	void FileBrowserModal::ConfirmSave()
	{
		const std::string extension = m_request.Extensions.empty() ? std::string() : m_request.Extensions.front();
		const SaveName saveName = ResolveSaveName(m_folder, m_fileName, extension);
		if (!saveName.Error.empty())
		{
			m_saveError = saveName.Error;
		}
		else if (saveName.Path.Exists())
		{
			m_pendingReplace = saveName.Path;
		}
		else
		{
			Choose(saveName.Path);
		}
	}

	void FileBrowserModal::Choose(Filepath file)
	{
		// Moved out first, so a callback that opens the browser again starts from a clean request
		FileBrowserRequest request = std::move(m_request);
		m_request = FileBrowserRequest();
		Close();

		if (request.OnChosen)
		{
			request.OnChosen(file);
		}
		else if (request.RequesterID != 0)
		{
			m_resultRequester = request.RequesterID;
			m_result = file;
		}
	}

	void FileBrowserModal::Cancel()
	{
		// Moved out first, like Choose
		std::function<void()> cancelled = std::move(m_request.OnCancelled);
		m_request = FileBrowserRequest();
		Close();

		if (cancelled)
		{
			cancelled();
		}
	}

	void FileBrowserModal::SetFileName(const std::string& name)
	{
		strncpy_s(m_fileName, name.c_str(), _TRUNCATE);
		m_saveError.clear();
		m_pendingReplace = Filepath();
	}

	void FileBrowserModal::Close()
	{
		ImGui::CloseCurrentPopup();
		m_isOpen = false;
		m_entries.clear();
		m_grid.ReleaseThumbnails();
	}

	const FolderEntry* FileBrowserModal::GetSelected() const
	{
		const Filepath& selectedPath = m_grid.GetSelected();
		if (!selectedPath.IsValid())
		{
			return nullptr;
		}

		const auto it = std::find_if(m_entries.begin(), m_entries.end(),
			[&selectedPath](const FolderEntry& entry) { return entry.Path.GetRelativePath() == selectedPath.GetRelativePath(); });
		return it != m_entries.end() ? &*it : nullptr;
	}
}
