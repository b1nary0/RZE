#include <StdAfx.h>
#include <Game/World/GameObjectComponents/DirectionalLightComponent.h>

#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/TransformComponent.h>

namespace
{
	// Length of the editor debug line showing which way the light points
	constexpr float k_directionLineLength = 5.0f;

	// The light shines along the transform's local -Y axis, so a zero rotation is a straight-down sun
	Vector3D CalculateLightDirection(const Vector3D& eulerRotationDegrees)
	{
		const Matrix4x4 rotation = Matrix4x4::CreateInPlace(Vector3D(), Vector3D(1.0f), eulerRotationDegrees);
		const Vector4D direction = rotation * Vector4D(0.0f, -1.0f, 0.0f, 0.0f);

		return Vector3D(direction.X(), direction.Y(), direction.Z()).Normalized();
	}
}

void DirectionalLightComponent::OnAddToScene()
{
	GameObjectComponentPtr<TransformComponent> transformComponent = GetOwner()->GetComponent<TransformComponent>();

	m_lightObject = RZE().GetRenderEngine().CreateLightObject();
	m_lightObject->SetDirection(CalculateLightDirection(transformComponent->GetRotation()));
	m_lightObject->SetColour(m_lightColour);
	m_lightObject->SetStrength(m_lightStrength);
}

void DirectionalLightComponent::OnRemoveFromScene()
{
	RZE().GetRenderEngine().DestroyLightObject(m_lightObject);
}

void DirectionalLightComponent::Update()
{
	if (m_lightObject != nullptr)
	{
		GameObjectComponentPtr<TransformComponent> transformComponent = GetOwner()->GetComponent<TransformComponent>();

		const Vector3D direction = CalculateLightDirection(transformComponent->GetRotation());

		m_lightObject->SetDirection(direction);
		m_lightObject->SetColour(m_lightColour);
		m_lightObject->SetStrength(m_lightStrength);

		// Position only matters for this gizmo; a directional light lights everything from the same direction
		const Vector3D& position = transformComponent->GetPosition();
		RZE().GetRenderEngine().DrawLine(position, position + direction * k_directionLineLength);
	}
}

void DirectionalLightComponent::Serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer)
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

void DirectionalLightComponent::Deserialize(const rapidjson::Value& data)
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
	
	ImGui::Text("Colour");
	ImGui::ColorEdit4("##directionallightcomponent_color", colorValues, ImGuiInputTextFlags_EnterReturnsTrue);

	ImGui::Text("Strength");
	ImGui::InputFloat("##directionallightcomponent_strength", &m_lightStrength, 0.005f, 0.05f, "%.2f", ImGuiInputTextFlags_EnterReturnsTrue);
}
