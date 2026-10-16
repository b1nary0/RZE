#include <UI/Settings/FrameRateSettingsPage.h>

#include <UI/Settings/SettingsPageRegistry.h>

#include <RZE.h>

#include <EngineCore/Config/FrameSettings.h>
#include <EngineCore/Timing/FramePacer.h>

#include <ImGui/imgui.h>

#include <cmath>
#include <string>

namespace
{
	// Font sizes across, so the options line up under each other
	constexpr float k_labelWidth = 9.0f;

	// Frames a second; 0 is Off. Engine.ini can set any other rate.
	constexpr int k_maxFrameRateOptions[] = { 0, 30, 60, 120, 144, 240 };
	constexpr int k_unfocusedFrameRateOptions[] = { 0, 10, 15, 30, 60 };

	// A label, then one radio button per option on its line. True when an option was picked.
	template <size_t Count>
	bool DrawFrameRateOptions(const char* label, const char* tooltip, float optionsX, const int (&options)[Count], double& frameRate)
	{
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(label);
		ImGui::SetItemTooltip("%s", tooltip);
		ImGui::SameLine(optionsX);
		ImGui::Dummy(ImVec2(0.0f, 0.0f));

		bool isPicked = false;
		ImGui::PushID(label);
		for (const int option : options)
		{
			ImGui::SameLine();
			const std::string optionLabel = option == 0 ? std::string("Off") : std::to_string(option);
			if (ImGui::RadioButton(optionLabel.c_str(), std::lround(frameRate) == option))
			{
				frameRate = static_cast<double>(option);
				isPicked = true;
			}
		}
		ImGui::PopID();

		return isPicked;
	}
}

namespace Editor
{
	REGISTER_SETTINGS_PAGE(FrameRateSettingsPage);

	void FrameRateSettingsPage::Draw(EditorContext&)
	{
		FrameSettings settings = RZE().GetFramePacer().GetSettings();
		const float optionsX = ImGui::GetCursorPosX() + ImGui::GetFontSize() * k_labelWidth;
		bool isChanged = false;

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Vsync");
		ImGui::SetItemTooltip("Presents on the display's refresh, so frames don't tear and run no faster than it");
		ImGui::SameLine(optionsX);
		isChanged |= ImGui::Checkbox("##VSync", &settings.VSync);

		isChanged |= DrawFrameRateOptions("Max frame rate", "Frames a second at most. Off leaves it to vsync, or uncapped without it.",
			optionsX, k_maxFrameRateOptions, settings.MaxFrameRate);
		isChanged |= DrawFrameRateOptions("While unfocused", "Frames a second while another window is in front or the editor is minimised. Off runs as usual.",
			optionsX, k_unfocusedFrameRateOptions, settings.UnfocusedFrameRate);

		if (isChanged)
		{
			RZE().SetFrameSettings(settings);
		}

		ImGui::Spacing();
		ImGui::TextDisabled("These last until the editor closes; Engine.ini's [FrameSettings] sets them at start-up.");
		ImGui::TextDisabled("The status bar shows the frame time, frames a second and idle time.");
	}
}
