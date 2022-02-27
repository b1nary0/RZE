#include <StdAfx.h>
#include <Game/World/GameObjectComponents/DirectionalLightComponent.h>

#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/TransformComponent.h>

void DirectionalLightComponent::OnAddToScene()
{
	TransformComponent* const transformComponent = GetOwner()->GetComponent<TransformComponent>();

	m_lightObject = RZE::GetRenderEngine().CreateLightObject();
	m_lightObject->SetPosition(transformComponent->GetPosition());
	m_lightObject->SetColour(m_lightColour);
	m_lightObject->SetStrength(m_lightStrength);
}

void DirectionalLightComponent::OnRemoveFromScene()
{
	RZE::GetRenderEngine().DestroyLightObject(m_lightObject);
}

void DirectionalLightComponent::Update()
{
	if (m_lightObject)
	{
		TransformComponent* const transformComponent = GetOwner()->GetComponent<TransformComponent>();

		m_lightObject->SetPosition(transformComponent->GetPosition());
		m_lightObject->SetColour(m_lightColour);
		m_lightObject->SetStrength(m_lightStrength);
	}
}

void DirectionalLightComponent::Save(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer)
{
	writer.String("DirectionalLightComponent");
	writer.StartObject();
	{
		writer.Key("LightStrength");
		writer.Double(m_lightStrength);

		writer.Key("LightColour");
		writer.StartArray();
		{
			for (int i = 0; i < 3; ++i)
			{
				writer.Double(m_lightColour[i]);
			}
		}
		writer.EndArray();
	}
	writer.EndObject();
}

void DirectionalLightComponent::Load(const rapidjson::Value& data)
{
	m_lightStrength = data["LightStrength"].GetFloat();
	m_lightColour = Vector3D(
		data["LightColour"][0].GetFloat(), 
		data["LightColour"][1].GetFloat(), 
		data["LightColour"][2].GetFloat());
}

void DirectionalLightComponent::OnEditorInspect()
{
	float* colorValues = const_cast<float*>(&m_lightColour.GetInternalVec().x);
	
	ImGui::Text("Color");
	ImGui::InputFloat4("##directionallightcomponent_color", colorValues, "%.2f", ImGuiInputTextFlags_EnterReturnsTrue);

	ImGui::Text("Strength");
	ImGui::InputFloat("##directionallightcomponent_strength", &m_lightStrength, 0.005f, 0.05f, "%.2f", ImGuiInputTextFlags_EnterReturnsTrue);
}
