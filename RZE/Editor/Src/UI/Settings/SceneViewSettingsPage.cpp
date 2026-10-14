#include <UI/Settings/SceneViewSettingsPage.h>

#include <UI/Framework/EditorContext.h>
#include <UI/Gizmo/GizmoSnapper.h>
#include <UI/Settings/SettingsPageRegistry.h>

#include <ImGui/imgui.h>

namespace
{
	// Font sizes across, so the options line up under each other
	constexpr float k_labelWidth = 7.0f;
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
		ImGui::Dummy(ImVec2(0.0f, 0.0f));
		GizmoSnapper::DrawSnapSizeOptions(settings);

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Rotate step");
		ImGui::SameLine(optionsX);
		ImGui::Dummy(ImVec2(0.0f, 0.0f));
		GizmoSnapper::DrawRotationStepOptions(settings);

		ImGui::Spacing();
		ImGui::TextDisabled("Press X in the scene view to turn snapping on or off.");
	}
}
