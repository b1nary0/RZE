#include <UI/Panels/ProjectSettingsPanel.h>

#include <UI/Framework/EditorUI.h>
#include <UI/Framework/PanelRegistry.h>
#include <UI/Inspector/PropertyDrawers.h>

#include <Graphics/Texture2D.h>
#include <Windowing/WindowIcon.h>

#include <Utils/DebugUtils/Debug.h>

#include <ImGui/imgui.h>

#include <algorithm>
#include <cstring>
#include <iterator>

namespace
{
	constexpr const char* k_gameConfigPath = "Config/Game.ini";

	// Pixels: the small and large icon sizes at 100% scaling, and a large taskbar's
	constexpr int k_previewSizes[] = { 16, 32, 48 };
	// The row's thumbnail, and what its hover preview shows
	constexpr int k_thumbnailSize = 256;
	// Below this, the large sizes are scaled up
	constexpr int k_recommendedIconSize = 256;

	// Frees texels allocated with new[]
	void FreeTexels(void* texels)
	{
		delete[] static_cast<U8*>(texels);
	}

	// A GPU texture of image, which shows a frame or so later
	PropertyTexture MakeTexture(const IconImage& image)
	{
		U8* const texels = new U8[image.Texels.size()];
		std::memcpy(texels, image.Texels.data(), image.Texels.size());

		Texture2D texture;
		texture.Load(Rendering::TexelBuffer(texels, &FreeTexels), image.Width, image.Height);

		PropertyTexture preview;
		preview.Handle = texture.GetPlatformObject();
		preview.Size = texture.GetDimensions();
		return preview;
	}
}

namespace Editor
{
	REGISTER_EDITOR_PANEL(ProjectSettingsPanel);

	PanelDesc ProjectSettingsPanel::Describe()
	{
		PanelDesc desc;
		desc.Name = "Project Settings";
		desc.MenuPath = "Edit/Project Settings";
		desc.DefaultDock = EPanelDock::Floating;
		desc.DefaultSize = ImVec2(560.0f, 300.0f);
		desc.DefaultOpen = false;
		desc.WindowFlags = ImGuiWindowFlags_NoCollapse;
		return desc;
	}

	void ProjectSettingsPanel::Initialize(EditorContext& context)
	{
		m_gameConfig.Load(Filepath(k_gameConfigPath));

		m_iconProperty.Name = "Icon";
		m_iconProperty.Type = EPropertyType::AssetPath;
		m_iconProperty.Asset("Icon", "Assets", "*.png").Clearable();
		m_iconProperty.GetThumbnail = [this](const void* object) { return m_thumbnail; };

		MakePreviews();
	}

	void ProjectSettingsPanel::Draw(EditorContext& context)
	{
		const float labelColumnWidth = EditorUI::CalcLabelColumnWidth(ImGui::CalcTextSize("Icon").x);
		if (!EditorUI::BeginPropertyTable("##ProjectSettings", labelColumnWidth))
		{
			return;
		}

		EditorUI::PropertySectionRow("Game");
		EditorUI::PropertyRow("Icon", "The game window's icon, in the taskbar and Alt+Tab. Without one it shows the RZE icon.");
		DrawIcon(context);

		EditorUI::EndPropertyTable();
	}

	void ProjectSettingsPanel::DrawIcon(EditorContext& context)
	{
		ImGui::PushID("Icon");
		PropertyValue value = m_gameConfig.GetIcon();
		if (PropertyDrawers::Find(EPropertyType::AssetPath)(context, m_iconProperty, this, value, false))
		{
			SetIcon(std::get<Filepath>(value));
		}
		ImGui::PopID();

		if (!m_gameConfig.GetIcon().IsValid())
		{
			ImGui::TextDisabled("The game shows the RZE icon.");
		}
		else if (m_iconWidth == 0)
		{
			ImGui::TextColored(EditorUI::k_errorColour, "Couldn't read this image. The game shows the RZE icon.");
		}
		else
		{
			// Bottom-aligned, as they'd sit on a taskbar
			const float bottom = ImGui::GetCursorPosY() + static_cast<float>(k_previewSizes[std::size(k_previewSizes) - 1]);
			for (size_t i = 0; i < m_previews.size(); ++i)
			{
				if (i > 0)
				{
					ImGui::SameLine();
				}
				ImGui::SetCursorPosY(bottom - static_cast<float>(k_previewSizes[i]));
				PropertyDrawers::DrawThumbnail(m_previews[i], static_cast<float>(k_previewSizes[i]));
			}

			if (m_iconWidth != m_iconHeight)
			{
				ImGui::TextDisabled("It isn't square, so it's centred with clear space around it.");
			}
			if (std::max(m_iconWidth, m_iconHeight) < k_recommendedIconSize)
			{
				ImGui::TextDisabled("It's %d x %d. %d x %d or larger stays sharp on high-DPI screens.", m_iconWidth, m_iconHeight,
					k_recommendedIconSize, k_recommendedIconSize);
			}
		}

		if (!m_saveError.empty())
		{
			ImGui::TextColored(EditorUI::k_errorColour, "%s", m_saveError.c_str());
		}
	}

	void ProjectSettingsPanel::SetIcon(const Filepath& icon)
	{
		m_gameConfig.SetIcon(icon);
		if (m_gameConfig.Save())
		{
			m_saveError.clear();
		}
		else
		{
			m_saveError = std::string("Couldn't save ") + k_gameConfigPath + ". Is it open elsewhere, or read-only?";
			RZE_LOG_ARGS("%s", m_saveError.c_str());
		}

		MakePreviews();
	}

	void ProjectSettingsPanel::MakePreviews()
	{
		m_previews.fill(PropertyTexture());
		m_thumbnail = PropertyTexture();
		m_iconWidth = 0;
		m_iconHeight = 0;

		if (!m_gameConfig.GetIcon().IsValid())
		{
			return;
		}

		const IconImage image = WindowIcon::ReadImage(m_gameConfig.GetIcon());
		if (image.IsEmpty())
		{
			return;
		}

		m_iconWidth = image.Width;
		m_iconHeight = image.Height;
		for (size_t i = 0; i < m_previews.size(); ++i)
		{
			m_previews[i] = MakeTexture(WindowIcon::MakeSquare(image, k_previewSizes[i]));
		}
		m_thumbnail = MakeTexture(WindowIcon::MakeSquare(image, k_thumbnailSize));
	}
}
