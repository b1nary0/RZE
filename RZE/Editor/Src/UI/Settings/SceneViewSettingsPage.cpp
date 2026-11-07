#include <UI/Settings/SceneViewSettingsPage.h>

#include <UI/Framework/EditorContext.h>
#include <UI/Framework/EditorUI.h>
#include <UI/Gizmo/GizmoSnapper.h>
#include <UI/Settings/SettingsPageRegistry.h>

#include <RZE.h>

#include <Graphics/AmbientOcclusion.h>
#include <Graphics/AntiAliasing.h>
#include <Graphics/RenderEngine.h>

#include <imgui/imgui.h>

#include <algorithm>

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
		ImGui::SetItemTooltip("How jagged edges are smoothed. FXAA blends across the edges it finds in the final image: the cheapest, but it softens some texture detail. SMAA works out each edge's shape, so it keeps textures sharp, and from High it handles diagonals and corners; it takes three passes.");
		ImGui::SameLine(optionsX);
		ImGui::Dummy(ImVec2(0.0f, 0.0f));
		RenderView& mainView = RZE().GetRenderEngine().GetMainView();
		size_t antiAliasingMethod = static_cast<size_t>(mainView.AntiAliasing.Method);
		if (EditorUI::RadioButtonRow("AntiAliasing", GetAntiAliasingMethodNames(), antiAliasingMethod))
		{
			mainView.AntiAliasing.Method = static_cast<EAntiAliasingMethod>(antiAliasingMethod);
		}

		// Quality means nothing with anti-aliasing off, so it's shown but greyed out
		ImGui::BeginDisabled(mainView.AntiAliasing.Method == EAntiAliasingMethod::Off);
		ImGui::AlignTextToFramePadding();
		// Indented, as part of the Anti-aliasing setting
		ImGui::Indent();
		ImGui::TextUnformatted("Quality");
		ImGui::SetItemTooltip("How hard anti-aliasing works. Higher qualities catch fainter edges and follow long ones further; FXAA's also soften more of the sparkle on thin parts. Ultra is SMAA's only: FXAA uses High.");
		ImGui::Unindent();
		ImGui::SameLine(optionsX);
		ImGui::Dummy(ImVec2(0.0f, 0.0f));
		// Only the method's qualities. One it doesn't have (FXAA's Ultra) shows as its highest, and stays set for a
		// method that has it.
		const size_t highestQuality = static_cast<size_t>(GetHighestQuality(mainView.AntiAliasing.Method));
		size_t antiAliasingQuality = std::min(static_cast<size_t>(mainView.AntiAliasing.Quality), highestQuality);
		if (EditorUI::RadioButtonRow("AntiAliasingQuality", GetAntiAliasingQualityNames(), highestQuality + 1, antiAliasingQuality))
		{
			mainView.AntiAliasing.Quality = static_cast<EAntiAliasingQuality>(antiAliasingQuality);
		}
		ImGui::EndDisabled();

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Ambient occlusion");
		ImGui::SetItemTooltip("How much creases and corners darken the light that isn't direct (GTAO). Higher levels search more directions around each pixel, for less noise.");
		ImGui::SameLine(optionsX);
		ImGui::Dummy(ImVec2(0.0f, 0.0f));
		size_t ambientOcclusion = static_cast<size_t>(mainView.AmbientOcclusion);
		if (EditorUI::RadioButtonRow("AmbientOcclusion", GetAmbientOcclusionNames(), ambientOcclusion))
		{
			mainView.AmbientOcclusion = static_cast<EAmbientOcclusion>(ambientOcclusion);
		}

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Global illumination");
		ImGui::SetItemTooltip("Indirect light from the scene's irradiance probes, which capture the scene around them and bounce the lights' light. Off, the ambient light is a flat sky and ground.");
		ImGui::SameLine(optionsX);
		ImGui::Checkbox("##GlobalIllumination", &mainView.GlobalIllumination);

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
