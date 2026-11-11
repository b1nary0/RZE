#include <UI/Settings/GraphicsSettingsPage.h>

#include <UI/Framework/EditorUI.h>
#include <UI/Settings/SettingsPageRegistry.h>

#include <RZE.h>

#include <Graphics/AmbientOcclusion.h>
#include <Graphics/AntiAliasing.h>
#include <Graphics/RenderEngine.h>

#include <Utils/EnumNames.h>

#include <imgui/imgui.h>

#include <algorithm>

namespace
{
	// Font sizes across, so the options line up under each other
	constexpr float k_labelWidth = 11.0f;

	// What Show offers, in ERenderViewDebug's order. These are only the editor's labels: Engine.ini doesn't set it.
	constexpr const char* k_showNames[] = { "Lit", "Ambient occlusion", "Indirect light" };

	const EnumNames& GetShowNames()
	{
		static const EnumNames names(k_showNames);
		return names;
	}

	// A row's label, with tooltip on hover, then moves to where its option goes
	void DrawLabel(const char* label, const char* tooltip, float optionsX)
	{
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(label);
		ImGui::SetItemTooltip("%s", tooltip);
		ImGui::SameLine(optionsX);
	}
}

namespace Editor
{
	REGISTER_SETTINGS_PAGE(GraphicsSettingsPage);

	void GraphicsSettingsPage::Draw(EditorContext&)
	{
		const float optionsX = ImGui::GetCursorPosX() + ImGui::GetFontSize() * k_labelWidth;
		RenderView& mainView = RZE().GetRenderEngine().GetMainView();

		DrawLabel("Anti-aliasing", "How jagged edges are smoothed. FXAA blends across the edges it finds in the final image: the cheapest, but it softens some texture detail. SMAA works out each edge's shape, so it keeps textures sharp, and from High it handles diagonals and corners; it takes three passes.", optionsX);
		size_t antiAliasingMethod = static_cast<size_t>(mainView.AntiAliasing.Method);
		if (EditorUI::EnumCombo("##AntiAliasing", GetAntiAliasingMethodNames(), antiAliasingMethod))
		{
			mainView.AntiAliasing.Method = static_cast<EAntiAliasingMethod>(antiAliasingMethod);
		}

		// Quality means nothing with anti-aliasing off, so it's shown but greyed out
		ImGui::BeginDisabled(mainView.AntiAliasing.Method == EAntiAliasingMethod::Off);
		// Indented, as part of the Anti-aliasing setting. Unindented before moving to the option, which Unindent would
		// otherwise move back left.
		ImGui::AlignTextToFramePadding();
		ImGui::Indent();
		ImGui::TextUnformatted("Quality");
		ImGui::SetItemTooltip("How hard anti-aliasing works. Higher qualities catch fainter edges and follow long ones further; FXAA's also soften more of the sparkle on thin parts. Ultra is SMAA's only: FXAA uses High.");
		ImGui::Unindent();
		ImGui::SameLine(optionsX);
		// Only the method's qualities. One it doesn't have (FXAA's Ultra) shows as its highest, and stays set for a
		// method that has it.
		const size_t highestQuality = static_cast<size_t>(GetHighestQuality(mainView.AntiAliasing.Method));
		size_t antiAliasingQuality = std::min(static_cast<size_t>(mainView.AntiAliasing.Quality), highestQuality);
		if (EditorUI::EnumCombo("##AntiAliasingQuality", GetAntiAliasingQualityNames(), highestQuality + 1, antiAliasingQuality))
		{
			mainView.AntiAliasing.Quality = static_cast<EAntiAliasingQuality>(antiAliasingQuality);
		}
		ImGui::EndDisabled();

		DrawLabel("Ambient occlusion", "How much creases and corners darken the light that isn't direct (GTAO). Higher levels search more directions around each pixel, for less noise.", optionsX);
		size_t ambientOcclusion = static_cast<size_t>(mainView.AmbientOcclusion);
		if (EditorUI::EnumCombo("##AmbientOcclusion", GetAmbientOcclusionNames(), ambientOcclusion))
		{
			mainView.AmbientOcclusion = static_cast<EAmbientOcclusion>(ambientOcclusion);
		}

		DrawLabel("Global illumination", "Indirect light from the scene's irradiance probes, which capture the scene around them and bounce the lights' light. Off, the ambient light is a flat sky and ground.", optionsX);
		ImGui::Checkbox("##GlobalIllumination", &mainView.GlobalIllumination);

		DrawLabel("Show", "What the scene view shows: the lit scene, or one part of its lighting alone. Ambient occlusion is white where nothing occludes; indirect light is what the probes or the sky would put on a white surface.", optionsX);
		size_t show = static_cast<size_t>(mainView.Debug);
		if (EditorUI::EnumCombo("##Show", GetShowNames(), show))
		{
			mainView.Debug = static_cast<ERenderViewDebug>(show);
		}

		DrawLabel("Auto exposure", "Brightens or darkens the image to the scene's brightness, adapting over time like an eye. Off, the exposure is fixed at 1 (plus the camera's exposure compensation).", optionsX);
		ImGui::Checkbox("##AutoExposure", &mainView.AutoExposure);

		ImGui::Spacing();
		ImGui::TextDisabled("These last until the editor closes; Engine.ini's [Rendering] sets them at start-up.");
	}
}
