#include <StdAfx.h>
#include <Game/World/GameObjectComponents/LightComponent.h>

#include <Game/GameScene.h>
#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/TransformComponent.h>

#include <Graphics/RenderScene.h>

#include <cstring>
#include <iterator>

namespace
{
	// Length of the editor debug line showing which way a directional light points
	constexpr float k_directionLineLength = 5.0f;

	// How Serialize writes each ELightType, in the order of its values
	constexpr const char* k_lightTypeNames[] = { "Directional", "Point" };
}

void LightComponent::Initialize()
{
	m_renderScene = &GetOwner()->GetScene().GetRenderScene();
}

void LightComponent::OnAddToScene()
{
	m_lightObject = m_renderScene->CreateLightObject(m_lightType);
	// Fills in the rest straight away rather than on the next Update
	Update();
}

void LightComponent::OnRemoveFromScene()
{
	m_renderScene->DestroyLightObject(m_lightObject);
}

void LightComponent::Update()
{
	if (m_lightObject != nullptr)
	{
		GameObjectComponentPtr<TransformComponent> transformComponent = GetOwner()->GetComponent<TransformComponent>();

		const Vector3D& position = transformComponent->GetPosition();
		const Vector3D direction = transformComponent->GetUp() * -1.0f;

		m_lightObject->SetType(m_lightType);
		m_lightObject->SetPosition(position);
		m_lightObject->SetDirection(direction);
		m_lightObject->SetRange(m_range);
		m_lightObject->SetColour(m_lightColour);
		m_lightObject->SetStrength(m_lightStrength);

		if (m_lightType == ELightType::Directional)
		{
			// Position only matters for this gizmo; a directional light lights everything from the same direction
			m_renderScene->DrawLine(position, position + direction * k_directionLineLength);
		}
	}
}

void LightComponent::Serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer)
{
	writer.String("LightComponent");
	writer.StartObject();
	{
		writer.Key("Type");
		writer.String(k_lightTypeNames[static_cast<int>(m_lightType)]);

		writer.Key("Range");
		writer.Double(m_range);

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

void LightComponent::Deserialize(const rapidjson::Value& data)
{
	// #TODO(LegacyLights) Only DirectionalLightComponent data has no type, so a missing one means directional. Goes
	// once every scene is saved as LightComponent; a missing type is then the default, a point light.
	m_lightType = ELightType::Directional;
	if (data.HasMember("Type"))
	{
		const char* const typeName = data["Type"].GetString();
		for (int i = 0; i < static_cast<int>(std::size(k_lightTypeNames)); ++i)
		{
			if (std::strcmp(typeName, k_lightTypeNames[i]) == 0)
			{
				m_lightType = static_cast<ELightType>(i);
			}
		}
	}

	if (data.HasMember("Range"))
	{
		m_range = data["Range"].GetFloat();
	}

	m_lightStrength = data["LightStrength"].GetFloat();
	m_lightColour = Vector3D(
		data["LightColour"][0].GetFloat(),
		data["LightColour"][1].GetFloat(),
		data["LightColour"][2].GetFloat());
}

void LightComponent::DescribeProperties(PropertyBuilder<LightComponent>& properties)
{
	properties.Enum<ELightType>("Type", { k_lightTypeNames[0], k_lightTypeNames[1] },
		[](const LightComponent& light) { return light.GetLightType(); },
		[](LightComponent& light, ELightType type) { light.SetLightType(type); });
	properties.Colour("Colour", &LightComponent::GetLightColour, &LightComponent::SetLightColour);
	properties.Float("Strength", &LightComponent::GetLightStrength, &LightComponent::SetLightStrength)
		.Range(0.0f, 100.0f).StepSize(0.05f).DisplayFormat("%.2f");
	properties.Float("Range", &LightComponent::GetRange, &LightComponent::SetRange)
		.Range(0.1f, 100.0f).StepSize(0.1f).DisplayFormat("%.1f m")
		.EnabledIf<LightComponent>([](const LightComponent& light) { return light.GetLightType() == ELightType::Point; }, "Only point lights have a range");
}
