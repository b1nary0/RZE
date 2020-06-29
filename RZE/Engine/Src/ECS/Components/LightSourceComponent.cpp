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

void LightSourceComponent::Load(const rapidjson::Value& data)
{
	LightType = static_cast<ELightType>(data["LightType"].GetUint());
	Color = Vector3D(data["Color"][0].GetFloat(), data["Color"][1].GetFloat(), data["Color"][2].GetFloat());
	Strength = data["Strength"].GetFloat();
}

void LightSourceComponent::Save(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer)
{
	writer.String("LightSourceComponent");
	writer.StartObject();
	{
		writer.Key("LightType");
		writer.Uint(static_cast<U32>(LightType));

		writer.Key("Color");
		writer.StartArray();
		{
			for (int i = 0; i < 3; ++i)
			{
				writer.Double(Color[i]);
			}
		}
		writer.EndArray();

		writer.Key("Strength");
		writer.Double(Strength);
	}
	writer.EndObject();
}
