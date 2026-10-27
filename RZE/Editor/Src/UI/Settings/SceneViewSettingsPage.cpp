#include <UI/Settings/SceneViewSettingsPage.h>

#include <UI/Framework/EditorContext.h>
#include <UI/Gizmo/GizmoSnapper.h>
#include <UI/Settings/SettingsPageRegistry.h>

#include <RZE.h>

#include <Graphics/AmbientOcclusion.h>
#include <Graphics/AntiAliasing.h>
#include <Graphics/RenderEngine.h>

#include <ImGui/imgui.h>

#include <string>

namespace
{
	// Font sizes across, so the options line up under each other
	constexpr float k_labelWidth = 11.0f;
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

		ImGui::SeparatorText("Rendering");

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Anti-aliasing");
		ImGui::SetItemTooltip("How much jagged edges are smoothed (FXAA). Higher levels follow long edges further, catch fainter ones, and soften more of the sparkle on thin parts.");
		ImGui::SameLine(optionsX);
		ImGui::Dummy(ImVec2(0.0f, 0.0f));
		RenderView& mainView = RZE().GetRenderEngine().GetMainView();
		for (int level = 0; level < static_cast<int>(EAntiAliasing::Count); ++level)
		{
			ImGui::SameLine();
			if (ImGui::RadioButton(GetAntiAliasingName(static_cast<EAntiAliasing>(level)), mainView.AntiAliasing == static_cast<EAntiAliasing>(level)))
			{
				mainView.AntiAliasing = static_cast<EAntiAliasing>(level);
			}
		}

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Ambient occlusion");
		ImGui::SetItemTooltip("How much creases and corners darken the light that isn't direct (GTAO). Higher levels search more directions around each pixel, for less noise.");
		ImGui::SameLine(optionsX);
		ImGui::Dummy(ImVec2(0.0f, 0.0f));
		for (int level = 0; level < static_cast<int>(EAmbientOcclusion::Count); ++level)
		{
			ImGui::SameLine();
			// ## keeps their IDs apart from the anti-aliasing buttons, which have the same names
			const std::string label = std::string(GetAmbientOcclusionName(static_cast<EAmbientOcclusion>(level))) + "##AmbientOcclusion";
			if (ImGui::RadioButton(label.c_str(), mainView.AmbientOcclusion == static_cast<EAmbientOcclusion>(level)))
			{
				mainView.AmbientOcclusion = static_cast<EAmbientOcclusion>(level);
			}
		}

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Show");
		ImGui::SetItemTooltip("What the scene view shows: the lit scene, or one part of its lighting alone. Ambient occlusion is white where nothing occludes; indirect light is what the probes or the sky would put on a white surface.");
		ImGui::SameLine(optionsX);
		ImGui::Dummy(ImVec2(0.0f, 0.0f));
		struct DebugOption
		{
			const char* Label;
			ERenderViewDebug Debug;
		};
		for (const DebugOption& option : { DebugOption{ "Lit", ERenderViewDebug::Lit }, DebugOption{ "Ambient occlusion##Show", ERenderViewDebug::AmbientOcclusion }, DebugOption{ "Indirect light", ERenderViewDebug::IndirectDiffuse } })
		{
			ImGui::SameLine();
			if (ImGui::RadioButton(option.Label, mainView.Debug == option.Debug))
			{
				mainView.Debug = option.Debug;
			}
		}

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Auto exposure");
		ImGui::SetItemTooltip("Brightens or darkens the image to the scene's brightness, adapting over time like an eye. Off, the exposure is fixed at 1 (plus the camera's exposure compensation).");
		ImGui::SameLine(optionsX);
		ImGui::Checkbox("##AutoExposure", &mainView.AutoExposure);

		ImGui::Spacing();
		ImGui::TextDisabled("These last until the editor closes; Engine.ini's [Rendering] sets them at start-up.");
	}
}
