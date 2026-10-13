#include <UI/Modals/FileBrowserModal.h>

#include <UI/Framework/EditorUI.h>

#include <RZE.h>

#include <Graphics/Texture2D.h>

#include <Utils/StringUtils.h>

#include <ImGui/imgui_internal.h> // RenderTextEllipsis

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

	constexpr float k_gridPadding = 10.0f;
	constexpr float k_tileSpacing = 8.0f;
	constexpr float k_minTileSize = 95.0f;
	constexpr float k_maxTileSize = 234.0f;
	constexpr float k_tileRounding = 5.0f;
	constexpr float k_imageInset = 4.0f;
	constexpr float k_imageRounding = 4.0f;
	// Placeholder images narrower than this leave out the "No preview" note
	constexpr float k_minWidthForNote = 90.0f;

	// Images load synchronously, so a big folder fills in over a few frames rather than stalling one
	constexpr int k_maxLoadsPerFrame = 2;
	// The render thread draws a frame or so behind, so a texture that's stopped being shown lives this long
	constexpr int k_releaseDelayFrames = 3;

	const ImVec4 k_folderColour(0.86f, 0.70f, 0.36f, 1.0f);
	const ImVec4 k_warningColour(1.0f, 0.78f, 0.35f, 1.0f);
	const ImVec4 k_errorColour(1.0f, 0.45f, 0.40f, 1.0f);
}

namespace Editor
{
	void FileBrowserModal::Open(FileBrowserRequest request)
	{
		ReleaseThumbnails();

		// A request still open is replaced, so it's answered as cancelled once the new one is set up
		std::function<void()> replacedCancelled = IsOpen() ? std::move(m_request.OnCancelled) : nullptr;

		m_request = std::move(request);
		m_folder = m_request.RootFolder;
		m_selected = Filepath();
		m_search[0] = '\0';
		m_scrollToSelected = false;

		// Starts in the initial selection's folder if that's inside the root
		if (m_request.InitialSelection.IsValid())
		{
			const Filepath folder = m_request.InitialSelection.GetParentFolder();
			if (folder.IsInside(m_request.RootFolder))
			{
				m_folder = folder;
				m_selected = m_request.InitialSelection;
				m_scrollToSelected = true;
			}
		}

		Refresh();
		m_scrollToTop = !m_scrollToSelected;

		m_saveError.clear();
		m_pendingReplace = Filepath();
		if (IsSaving())
		{
			SetFileName(m_request.InitialSelection.IsValid() ? m_request.InitialSelection.GetStem() : m_request.DefaultFileName);
			m_focusFileName = true;
		}

		// Opened from Display, since a request can come from inside a menu or a panel, whose ID stack the popup mustn't use
		m_shouldOpen = true;

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

	void FileBrowserModal::RefreshThumbnails()
	{
		ReleaseThumbnails();
	}

	void FileBrowserModal::Display()
	{
		UpdatePendingReleases();
		m_loadsThisFrame = 0;

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
		ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(size, ImGuiCond_Always);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(k_windowPadding, k_windowPadding));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, k_windowRounding);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
		ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.11f, 0.11f, 0.12f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 1.0f, 1.0f, 0.10f));
		ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0.0f, 0.0f, 0.0f, 0.55f));

		const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize
			| ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
		const bool isVisible = ImGui::BeginPopupModal(k_popupID, nullptr, flags);

		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar(3);

		if (!isVisible)
		{
			// Closed by something other than the browser
			m_isOpen = false;
			ReleaseThumbnails();
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

		DrawBreadcrumb();

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
			m_scrollToTop = true;
		}
		m_isTextActive = ImGui::IsItemActive() || ImGui::IsItemDeactivated();

		ImGui::SetCursorPos(ImVec2(start.x, headerBottom));
		ImGui::Spacing();
	}

	void FileBrowserModal::DrawBreadcrumb()
	{
		// The root, then each folder below it down to the current one
		std::vector<Filepath> folders;
		for (Filepath folder = m_folder; folder.IsInside(m_request.RootFolder); folder = folder.GetParentFolder())
		{
			folders.insert(folders.begin(), folder);
		}

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, ImGui::GetStyle().ItemSpacing.y));
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.08f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 1.0f, 1.0f, 0.14f));

		Filepath navigateTo;
		for (size_t index = 0; index < folders.size(); ++index)
		{
			const std::string name = folders[index].GetFileName();

			if (index > 0)
			{
				ImGui::SameLine();
				ImGui::TextDisabled("/");
				ImGui::SameLine();
			}

			ImGui::PushID(static_cast<int>(index));
			if (index + 1 == folders.size())
			{
				ImGui::AlignTextToFramePadding();
				ImGui::TextDisabled("%s", name.c_str());
			}
			else if (ImGui::Button(name.c_str()))
			{
				navigateTo = folders[index];
			}
			ImGui::PopID();
		}

		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar(2);

		if (navigateTo.IsValid())
		{
			Navigate(navigateTo);
		}
	}

	void FileBrowserModal::DrawTiles()
	{
		const ImGuiStyle& style = ImGui::GetStyle();
		const bool hasNoticeRow = IsSaving() && !m_saveError.empty() && !m_pendingReplace.IsValid();
		const float footerHeight = ImGui::GetFrameHeight() + style.ItemSpacing.y * 2.0f + 6.0f
			+ (hasNoticeRow ? ImGui::GetTextLineHeightWithSpacing() : 0.0f);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(k_gridPadding, k_gridPadding));
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, k_tileRounding);
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.075f, 0.075f, 0.08f, 1.0f));
		const bool isChildVisible = ImGui::BeginChild("##tiles", ImVec2(0.0f, -footerHeight), ImGuiChildFlags_AlwaysUseWindowPadding);
		ImGui::PopStyleColor();
		ImGui::PopStyleVar(2);

		if (!isChildVisible)
		{
			ImGui::EndChild();
			return;
		}

		if (m_scrollToTop)
		{
			ImGui::SetScrollY(0.0f);
			m_scrollToTop = false;
		}

		const float lineHeight = ImGui::GetTextLineHeight();
		const ImVec2 tileSize(m_tileSize, m_tileSize + lineHeight * 2.0f + 7.0f);
		const float available = ImGui::GetContentRegionAvail().x;
		const int columns = std::max(1, static_cast<int>((available + k_tileSpacing) / (tileSize.x + k_tileSpacing)));

		HandleKeys(columns);
		if (!m_isOpen)
		{
			ImGui::EndChild();
			return;
		}

		if (m_entries.empty())
		{
			const char* const message = m_search[0] != '\0' ? "Nothing here matches the search" : "This folder is empty";
			const ImVec2 textSize = ImGui::CalcTextSize(message);
			const ImVec2 region = ImGui::GetContentRegionAvail();
			const ImVec2 cursor = ImGui::GetCursorPos();
			ImGui::SetCursorPos(ImVec2(cursor.x + (region.x - textSize.x) * 0.5f, cursor.y + (region.y - textSize.y) * 0.5f));
			ImGui::TextDisabled("%s", message);
			ImGui::EndChild();
			return;
		}

		// Centred, with whatever's left over split either side
		const float gridWidth = columns * tileSize.x + (columns - 1) * k_tileSpacing;
		const float startX = ImGui::GetCursorPosX() + std::max(0.0f, (available - gridWidth) * 0.5f);

		const int count = static_cast<int>(m_entries.size());
		const int rows = (count + columns - 1) / columns;
		const FolderEntry* const selected = GetSelected();
		const int selectedIndex = selected != nullptr ? static_cast<int>(selected - m_entries.data()) : -1;

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(k_tileSpacing, k_tileSpacing));

		int activated = -1;
		ImGuiListClipper clipper;
		clipper.Begin(rows, tileSize.y + k_tileSpacing);
		if (m_scrollToSelected && selectedIndex >= 0)
		{
			clipper.IncludeItemByIndex(selectedIndex / columns);
		}
		while (clipper.Step())
		{
			for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row)
			{
				for (int column = 0; column < columns; ++column)
				{
					const int index = row * columns + column;
					if (index >= count)
					{
						break;
					}

					if (column == 0)
					{
						ImGui::SetCursorPosX(startX);
					}
					else
					{
						ImGui::SameLine(startX + column * (tileSize.x + k_tileSpacing));
					}

					if (DrawTile(m_entries[index], index, tileSize))
					{
						activated = index;
					}

					if (m_scrollToSelected && index == selectedIndex)
					{
						ImGui::SetScrollHereY(0.5f);
						m_scrollToSelected = false;
					}
				}
			}
		}

		ImGui::PopStyleVar();
		ImGui::EndChild();

		// After the loop, since opening a folder replaces m_entries
		if (activated >= 0)
		{
			m_selected = m_entries[activated].Path;
			ConfirmSelection();
		}

		if (m_pendingAction)
		{
			const std::function<void(const Filepath&)> action = std::move(m_pendingAction);
			m_pendingAction = nullptr;
			action(m_pendingActionFile);
		}
	}

	bool FileBrowserModal::DrawTile(const FolderEntry& entry, int index, const ImVec2& size)
	{
		ImGui::PushID(index);

		ImGui::InvisibleButton("##tile", size, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
		const bool isHovered = ImGui::IsItemHovered();
		if (ImGui::IsItemActivated())
		{
			m_selected = entry.Path;
			if (IsSaving() && !entry.IsFolder)
			{
				SetFileName(entry.Path.GetStem());
			}
		}
		const bool isActivated = isHovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
		const bool isSelected = entry.Path.GetRelativePath() == m_selected.GetRelativePath();

		if (!entry.IsFolder && !m_request.TileActions.empty() && ImGui::BeginPopupContextItem("##actions"))
		{
			for (const FileBrowserTileAction& action : m_request.TileActions)
			{
				if (ImGui::MenuItem(action.Label.c_str()))
				{
					// Run after the tiles, since an action can change what the browser shows
					m_pendingAction = action.Run;
					m_pendingActionFile = entry.Path;
				}
			}
			ImGui::EndPopup();
		}
		else if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
		{
			ImGui::SetTooltip("%s", entry.Path.GetRelativePath().c_str());
		}

		const ImVec2 min = ImGui::GetItemRectMin();
		const ImVec2 max = ImGui::GetItemRectMax();
		ImDrawList* const drawList = ImGui::GetWindowDrawList();

		const ImU32 background = isSelected ? EditorUI::WithAlpha(EditorUI::k_accentColour, 0.18f) : ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, isHovered ? 0.07f : 0.03f));
		drawList->AddRectFilled(min, max, background, k_tileRounding);

		const ImVec2 imageMin(min.x + k_imageInset, min.y + k_imageInset);
		const ImVec2 imageMax(max.x - k_imageInset, imageMin.y + (size.x - k_imageInset * 2.0f));
		DrawTileImage(entry, imageMin, imageMax);
		if (!isHovered && !isSelected)
		{
			// Hovering or selecting a tile brightens its image
			drawList->AddRectFilled(imageMin, imageMax, IM_COL32(0, 0, 0, 40), k_imageRounding);
		}

		// The name, then when it last changed
		const float lineHeight = ImGui::GetTextLineHeight();
		const std::string name = entry.IsFolder ? entry.Name : entry.Path.GetStem();
		const std::string detail = entry.IsFolder ? std::string("Folder") : FormatTimeAgo(entry.ModifiedTime, std::chrono::system_clock::now());
		const float textLeft = min.x + k_imageInset + 2.0f;
		const float textRight = max.x - k_imageInset;
		const float nameY = imageMax.y + 4.0f;

		ImGui::RenderTextEllipsis(drawList, ImVec2(textLeft, nameY), ImVec2(textRight, nameY + lineHeight), textRight,
			name.c_str(), nullptr, nullptr);
		ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
		ImGui::RenderTextEllipsis(drawList, ImVec2(textLeft, nameY + lineHeight + 1.0f), ImVec2(textRight, nameY + lineHeight * 2.0f + 1.0f), textRight,
			detail.c_str(), nullptr, nullptr);
		ImGui::PopStyleColor();

		if (isSelected)
		{
			drawList->AddRect(min, max, EditorUI::WithAlpha(EditorUI::k_accentColour, 1.0f), k_tileRounding, 0, 2.0f);
		}
		else if (isHovered)
		{
			drawList->AddRect(min, max, ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.18f)), k_tileRounding, 0, 1.0f);
		}

		ImGui::PopID();
		return isActivated;
	}

	void FileBrowserModal::DrawTileImage(const FolderEntry& entry, const ImVec2& min, const ImVec2& max)
	{
		ImDrawList* const drawList = ImGui::GetWindowDrawList();
		const float width = max.x - min.x;
		const ImVec2 centre((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f);

		if (entry.IsFolder)
		{
			drawList->AddRectFilled(min, max, IM_COL32(40, 40, 44, 255), k_imageRounding);

			// A folder: a tab at the back, then the front
			const float folderWidth = width * 0.5f;
			const float folderHeight = width * 0.36f;
			const ImVec2 bodyMin(centre.x - folderWidth * 0.5f, centre.y - folderHeight * 0.5f);
			const ImVec2 bodyMax(centre.x + folderWidth * 0.5f, centre.y + folderHeight * 0.5f);
			const float tabHeight = folderHeight * 0.18f;
			drawList->AddRectFilled(ImVec2(bodyMin.x, bodyMin.y - tabHeight), ImVec2(bodyMin.x + folderWidth * 0.4f, bodyMin.y + tabHeight),
				EditorUI::WithAlpha(k_folderColour, 0.7f), tabHeight * 0.6f);
			drawList->AddRectFilled(bodyMin, bodyMax, EditorUI::WithAlpha(k_folderColour, 1.0f), folderHeight * 0.08f);
			return;
		}

		const Texture2D* const texture = GetThumbnail(entry.Path);
		if (texture != nullptr && texture->IsOnGPU())
		{
			// Fitted inside the square, keeping its shape
			const Vector2D dimensions = texture->GetDimensions();
			const float scale = std::min(width / dimensions.X(), width / dimensions.Y());
			const ImVec2 halfSize(dimensions.X() * scale * 0.5f, dimensions.Y() * scale * 0.5f);

			drawList->AddRectFilled(min, max, IM_COL32(20, 20, 22, 255), k_imageRounding);
			drawList->AddImageRounded((ImTextureID)(intptr_t)texture->GetPlatformObject().GetTextureData(),
				ImVec2(centre.x - halfSize.x, centre.y - halfSize.y), ImVec2(centre.x + halfSize.x, centre.y + halfSize.y),
				ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), IM_COL32_WHITE, k_imageRounding);
			return;
		}

		// No image: a colour picked by the file's extension, with the extension written large
		const std::string extension = entry.Path.GetExtension();
		const float hue = static_cast<float>(std::hash<std::string>()(extension) % 360) / 360.0f;
		drawList->AddRectFilled(min, max, ImColor::HSV(hue, 0.35f, 0.30f), k_imageRounding);

		const std::string label = StringUtils::ToUpper(extension.size() > 1 ? extension.substr(1) : std::string("FILE"));
		// Small tiles only have room for the extension
		const bool hasRoomForNote = width >= k_minWidthForNote;
		EditorUI::AddCentredText(drawList, ImVec2(centre.x, hasRoomForNote ? centre.y - width * 0.06f : centre.y), std::max(12.0f, width * 0.16f),
			IM_COL32(255, 255, 255, 215), label.c_str());
		if (hasRoomForNote)
		{
			EditorUI::AddCentredText(drawList, ImVec2(centre.x, centre.y + width * 0.15f), ImGui::GetFontSize(), IM_COL32(255, 255, 255, 110), "No preview");
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

	void FileBrowserModal::HandleKeys(int columns)
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
			return;
		}

		int step = 0;
		if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) { step = -1; }
		if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) { step = 1; }
		if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) { step = -columns; }
		if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) { step = columns; }
		if (step == 0 || m_entries.empty())
		{
			return;
		}

		// With nothing selected, any arrow starts at the first tile
		const FolderEntry* const selected = GetSelected();
		const int next = selected != nullptr
			? std::clamp(static_cast<int>(selected - m_entries.data()) + step, 0, static_cast<int>(m_entries.size()) - 1)
			: 0;
		m_selected = m_entries[next].Path;
		m_scrollToSelected = true;
	}

	void FileBrowserModal::Navigate(const Filepath& folder)
	{
		m_folder = folder;
		m_search[0] = '\0';
		Refresh();
		m_scrollToTop = true;
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
			m_selected = Filepath();
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
		ReleaseThumbnails();
	}

	const Texture2D* FileBrowserModal::GetThumbnail(const Filepath& file)
	{
		if (!m_request.ThumbnailFor)
		{
			return nullptr;
		}

		const Filepath image = m_request.ThumbnailFor(file);
		if (!image.IsValid())
		{
			return nullptr;
		}

		const std::string& key = image.GetRelativePath();
		auto it = m_thumbnails.find(key);
		if (it == m_thumbnails.end())
		{
			// ResourceHandler shares a loaded image by its path, so an earlier copy has to go before a fresh one loads
			const bool isBeingReleased = std::any_of(m_pendingReleases.begin(), m_pendingReleases.end(),
				[&key](const PendingRelease& pending) { return pending.Texture.GetResourcePath().GetRelativePath() == key; });
			if (isBeingReleased || m_loadsThisFrame >= k_maxLoadsPerFrame)
			{
				return nullptr;
			}

			++m_loadsThisFrame;
			ResourceHandle texture = image.Exists() ? RZE().GetResourceHandler().LoadResource<Texture2D>(image) : ResourceHandle::EmptyHandle();
			it = m_thumbnails.emplace(key, std::move(texture)).first;
		}

		return it->second.IsValid() ? RZE().GetResourceHandler().GetResource<Texture2D>(it->second) : nullptr;
	}

	void FileBrowserModal::ReleaseThumbnails()
	{
		for (auto& [path, texture] : m_thumbnails)
		{
			if (texture.IsValid())
			{
				m_pendingReleases.push_back({ std::move(texture), k_releaseDelayFrames });
			}
		}
		m_thumbnails.clear();
	}

	void FileBrowserModal::UpdatePendingReleases()
	{
		for (PendingRelease& pending : m_pendingReleases)
		{
			--pending.FramesLeft;
		}

		// Erasing drops each handle, which releases its texture once nothing else holds it
		m_pendingReleases.erase(std::remove_if(m_pendingReleases.begin(), m_pendingReleases.end(),
			[](const PendingRelease& pending) { return pending.FramesLeft <= 0; }), m_pendingReleases.end());
	}

	const FolderEntry* FileBrowserModal::GetSelected() const
	{
		if (!m_selected.IsValid())
		{
			return nullptr;
		}

		const auto it = std::find_if(m_entries.begin(), m_entries.end(),
			[this](const FolderEntry& entry) { return entry.Path.GetRelativePath() == m_selected.GetRelativePath(); });
		return it != m_entries.end() ? &*it : nullptr;
	}
}
