#include <UI/Settings/SceneViewSettingsPage.h>

#include <UI/Framework/EditorContext.h>
#include <UI/Framework/EditorUI.h>
#include <UI/Settings/SettingsPageRegistry.h>

#include <imgui/imgui.h>

#include <optional>
#include <string>
#include <vector>

namespace
{
	// Font sizes across, so the options line up under each other
	constexpr float k_labelWidth = 11.0f;

	// A drop-down of the options, labelled by format; one that isn't an option shows as itself. Returns the option
	// picked this frame.
	template <size_t Count>
	std::optional<float> DrawStepCombo(const char* id, const float (&options)[Count], float value, std::string (*format)(float))
	{
		std::vector<std::string> labels;
		size_t selected = Count;
		for (size_t option = 0; option < Count; ++option)
		{
			labels.push_back(format(options[option]));
			if (options[option] == value)
			{
				selected = option;
			}
		}

		if (Editor::EditorUI::Combo(id, labels, selected, format(value).c_str()))
		{
			return options[selected];
		}
		return std::nullopt;
	}
}

namespace Editor
{
	REGISTER_SETTINGS_PAGE(SceneViewSettingsPage);

	void SceneViewSettingsPage::Draw(EditorContext& context)
	{
		EditorSettings& settings = context.GetSettings();
		const float optionsX = ImGui::GetCursorPosX() + ImGui::GetFontSize() * k_labelWidth;

		ImGui::SeparatorText("Snapping");

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Move step");
		ImGui::SetItemTooltip("Also the grid's square size, and how far each paste is offset");
		ImGui::SameLine(optionsX);
		if (const std::optional<float> snapSize = DrawStepCombo("##SnapSize", EditorSettings::k_snapSizeOptions, settings.GetSnapSize(), &EditorUI::FormatMetres))
		{
			settings.SetSnapSize(*snapSize);
		}

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Rotate step");
		ImGui::SameLine(optionsX);
		if (const std::optional<float> rotationStep = DrawStepCombo("##RotationStep", EditorSettings::k_rotationStepOptions, settings.GetRotationStep(), &EditorUI::FormatDegrees))
		{
			settings.SetRotationStep(*rotationStep);
		}

		ImGui::Spacing();
		ImGui::TextDisabled("Press X in the scene view to turn snapping on or off.");
	}
}
