#include <StdAfx.h>
#include <ECS/Components/LightSourceComponent.h>

// #TODO(Temp for now)
static std::vector<const char*> kLightTypeNames = { "Directional", "Point !! Not Implemented W/ Shadows" };
static int kCurrentIndex = 0;

void LightSourceComponent::OnEditorInspect(Apollo::EntityID entityID)
{
	float* colorValues = const_cast<float*>(&Color.GetInternalVec().x);
	
	ImGui::Text("Light Type");
	ImGui::Combo("##lightsourcecomponent_lighttype", &kCurrentIndex, kLightTypeNames.data(), kLightTypeNames.size());

	if (ImGui::IsItemDeactivatedAfterEdit())
	{
		LightType = static_cast<ELightType>(kCurrentIndex);
	}

	ImGui::Text("Color");
	ImGui::DragFloat3("##lightsourcecomponent_color", colorValues, 0.0025f, 0.0f, 1.0f, "%.2f");

	ImGui::Text("Strength");
	ImGui::InputFloat("##lightsourcecomponent_strength", &Strength, 0.005f, 0.05f, "%.2f", ImGuiInputTextFlags_EnterReturnsTrue);
}