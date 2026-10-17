#include <UI/Widgets/FileTileGrid.h>

#include <UI/Framework/EditorUI.h>

#include <RZE.h>

#include <EngineCore/Resources/ResourcePreloadOperation.h>

#include <Graphics/Texture2D.h>

#include <Utils/StringUtils.h>

#include <ImGui/imgui_internal.h> // RenderTextEllipsis

#include <algorithm>
#include <chrono>

namespace
{
	constexpr float k_gridPadding = 10.0f;
	constexpr float k_tileSpacing = 8.0f;
	constexpr float k_tileRounding = 5.0f;
	constexpr float k_imageInset = 4.0f;
	constexpr float k_imageRounding = 4.0f;
	// Placeholder images narrower than this leave out the "No preview" note
	constexpr float k_minWidthForNote = 90.0f;

	const ImVec4 k_folderColour(0.86f, 0.70f, 0.36f, 1.0f);
}

namespace Editor
{
	void FileTileGrid::SetOptions(FileTileGridOptions options)
	{
		m_options = std::move(options);
	}

	FileTileGridResult FileTileGrid::Draw(const std::vector<FolderEntry>& entries, float tileSize, const ImVec2& size, const char* emptyText)
	{
		FileTileGridResult result;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(k_gridPadding, k_gridPadding));
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, k_tileRounding);
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.075f, 0.075f, 0.08f, 1.0f));
		// Not a handle for moving the window it's in, so a drag that misses a tile does nothing
		const bool isChildVisible = ImGui::BeginChild("##tiles", size, ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoMove);
		ImGui::PopStyleColor();
		ImGui::PopStyleVar(2);

		if (!isChildVisible)
		{
			ImGui::EndChild();
			return result;
		}

		if (m_scrollToTop)
		{
			ImGui::SetScrollY(0.0f);
			m_scrollToTop = false;
		}

		const float lineHeight = ImGui::GetTextLineHeight();
		const ImVec2 fullTileSize(tileSize, tileSize + lineHeight * 2.0f + 7.0f);
		const float available = ImGui::GetContentRegionAvail().x;
		const int columns = std::max(1, static_cast<int>((available + k_tileSpacing) / (fullTileSize.x + k_tileSpacing)));

		HandleArrowKeys(entries, columns);

		if (entries.empty())
		{
			const ImVec2 textSize = ImGui::CalcTextSize(emptyText);
			const ImVec2 region = ImGui::GetContentRegionAvail();
			const ImVec2 cursor = ImGui::GetCursorPos();
			ImGui::SetCursorPos(ImVec2(cursor.x + (region.x - textSize.x) * 0.5f, cursor.y + (region.y - textSize.y) * 0.5f));
			ImGui::TextDisabled("%s", emptyText);
			ImGui::EndChild();
			return result;
		}

		// Centred, with whatever's left over split either side
		const float gridWidth = columns * fullTileSize.x + (columns - 1) * k_tileSpacing;
		const float startX = ImGui::GetCursorPosX() + std::max(0.0f, (available - gridWidth) * 0.5f);

		const int count = static_cast<int>(entries.size());
		const int rows = (count + columns - 1) / columns;
		const int selectedIndex = FindSelectedIndex(entries);

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(k_tileSpacing, k_tileSpacing));

		ImGuiListClipper clipper;
		clipper.Begin(rows, fullTileSize.y + k_tileSpacing);
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
						ImGui::SameLine(startX + column * (fullTileSize.x + k_tileSpacing));
					}

					DrawTile(entries[index], index, fullTileSize, result);

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

		// After the tiles, since an action can change what the grid's owner shows
		if (m_pendingAction)
		{
			const std::function<void(const Filepath&)> action = std::move(m_pendingAction);
			m_pendingAction = nullptr;
			action(m_pendingActionFile);
		}

		return result;
	}

	void FileTileGrid::SetSelected(const Filepath& file, bool scrollTo)
	{
		m_selected = file;
		m_scrollToSelected = scrollTo && file.IsValid();
	}

	void FileTileGrid::RefreshThumbnail(const Filepath& image)
	{
		const auto it = m_thumbnails.find(image.GetRelativePath());
		if (it == m_thumbnails.end())
		{
			return;
		}

		// Released, so it's loaded again from the file next time a tile shows it
		if (it->second.Load != nullptr)
		{
			it->second.Load->RequestCancel();
		}
		m_thumbnails.erase(it);
	}

	void FileTileGrid::ReleaseThumbnails()
	{
		for (auto& [key, thumbnail] : m_thumbnails)
		{
			if (thumbnail.Load != nullptr)
			{
				thumbnail.Load->RequestCancel();
			}
		}

		// The ResourceHandler keeps a released texture until the render thread has drawn the frames that show it
		m_thumbnails.clear();
	}

	void FileTileGrid::DrawTile(const FolderEntry& entry, int index, const ImVec2& size, FileTileGridResult& result)
	{
		ImGui::PushID(index);

		ImGui::InvisibleButton("##tile", size, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
		const bool isHovered = ImGui::IsItemHovered();
		if (ImGui::IsItemActivated())
		{
			m_selected = entry.Path;
			result.Clicked = entry.Path;
		}
		if (isHovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			result.Activated = entry.Path;
		}
		const bool isSelected = entry.Path.GetRelativePath() == m_selected.GetRelativePath();

		if (!entry.IsFolder && !m_options.TileActions.empty() && ImGui::BeginPopupContextItem("##actions"))
		{
			for (const FileTileAction& action : m_options.TileActions)
			{
				if (ImGui::MenuItem(action.Label.c_str()))
				{
					// Run after the tiles, since an action can change what the grid's owner shows
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
	}

	void FileTileGrid::DrawTileImage(const FolderEntry& entry, const ImVec2& min, const ImVec2& max)
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

	void FileTileGrid::HandleArrowKeys(const std::vector<FolderEntry>& entries, int columns)
	{
		// Typing in a text field keeps its keys, and another window's arrow keys aren't the grid's
		if (ImGui::IsAnyItemActive() || !ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
		{
			return;
		}

		int step = 0;
		if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) { step = -1; }
		if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) { step = 1; }
		if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) { step = -columns; }
		if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) { step = columns; }
		if (step == 0 || entries.empty())
		{
			return;
		}

		// With nothing selected, any arrow starts at the first tile
		const int selectedIndex = FindSelectedIndex(entries);
		const int next = selectedIndex >= 0 ? std::clamp(selectedIndex + step, 0, static_cast<int>(entries.size()) - 1) : 0;
		m_selected = entries[next].Path;
		m_scrollToSelected = true;
	}

	const Texture2D* FileTileGrid::GetThumbnail(const Filepath& file)
	{
		if (!m_options.ThumbnailFor)
		{
			return nullptr;
		}

		const Filepath image = m_options.ThumbnailFor(file);
		if (!image.IsValid())
		{
			return nullptr;
		}

		const std::string& key = image.GetRelativePath();
		auto it = m_thumbnails.find(key);
		if (it == m_thumbnails.end())
		{
			it = m_thumbnails.emplace(key, Thumbnail()).first;
			Thumbnail& thumbnail = it->second;
			if (!image.Exists())
			{
				if (m_options.OnThumbnailMissing)
				{
					m_options.OnThumbnailMissing(file);
				}
			}
			else
			{
				// Taken straight away if something else has it loaded. Otherwise it loads in the background, each image
				// on its own so it's let go of as soon as it's in, and the tile shows the placeholder until then.
				thumbnail.Texture = RZE().GetResourceHandler().GetHandle(image);
				if (!thumbnail.Texture.IsValid())
				{
					ResourceDependencyList images;
					images.Add<Texture2D>(key);
					thumbnail.Load = std::make_shared<ResourcePreloadOperation>(std::move(images), EAsyncOperationFlags::Background | EAsyncOperationFlags::Hidden);
					RZE().GetAsyncOperationManager().Start(thumbnail.Load);
				}
			}
		}

		Thumbnail& thumbnail = it->second;
		if (thumbnail.Load != nullptr)
		{
			thumbnail.Texture = thumbnail.Load->GetHandle(image);
			// A load that fails is logged, and the tile keeps the placeholder
			if (thumbnail.Texture.IsValid() || thumbnail.Load->IsFinished())
			{
				thumbnail.Load = nullptr;
			}
		}

		return thumbnail.Texture.IsValid() ? RZE().GetResourceHandler().GetResource<Texture2D>(thumbnail.Texture) : nullptr;
	}

	int FileTileGrid::FindSelectedIndex(const std::vector<FolderEntry>& entries) const
	{
		if (!m_selected.IsValid())
		{
			return -1;
		}

		const auto it = std::find_if(entries.begin(), entries.end(),
			[this](const FolderEntry& entry) { return entry.Path.GetRelativePath() == m_selected.GetRelativePath(); });
		return it != entries.end() ? static_cast<int>(it - entries.begin()) : -1;
	}

	Filepath DrawBreadcrumb(const Filepath& root, const Filepath& folder)
	{
		// The root, then each folder below it down to the current one
		std::vector<Filepath> folders;
		for (Filepath current = folder; current.IsInside(root); current = current.GetParentFolder())
		{
			folders.insert(folders.begin(), current);
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

		return navigateTo;
	}
}
