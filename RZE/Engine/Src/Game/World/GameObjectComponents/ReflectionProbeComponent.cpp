#include <StdAfx.h>
#include <Game/World/GameObjectComponents/ReflectionProbeComponent.h>

#include <Game/GameScene.h>
#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/TransformComponent.h>

#include <Graphics/RenderScene.h>

void ReflectionProbeComponent::Initialize()
{
	m_renderScene = &GetOwner()->GetScene().GetRenderScene();
}

void ReflectionProbeComponent::OnAddToScene()
{
	m_reflectionProbe = m_renderScene->CreateReflectionProbe();
	// Fills in the rest straight away rather than on the next Update
	Update();
}

void ReflectionProbeComponent::OnRemoveFromScene()
{
	m_renderScene->DestroyReflectionProbe(m_reflectionProbe);
}

void ReflectionProbeComponent::Update()
{
	if (m_reflectionProbe != nullptr)
	{
		m_reflectionProbe->SetPosition(GetOwner()->GetTransformComponent()->GetPosition());
		m_reflectionProbe->SetExtents(m_size * 0.5f);
		m_reflectionProbe->SetBlendDistance(m_blendDistance);
	}
}

void ReflectionProbeComponent::Serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer)
{
	writer.String("ReflectionProbeComponent");
	writer.StartObject();
	{
		writer.Key("Size");
		writer.StartArray();
		{
			for (int i = 0; i < 3; ++i)
			{
				writer.Double(m_size[i]);
			}
		}
		writer.EndArray();

		writer.Key("BlendDistance");
		writer.Double(m_blendDistance);
	}
	writer.EndObject();
}

void ReflectionProbeComponent::Deserialize(const rapidjson::Value& data)
{
	if (data.HasMember("Size"))
	{
		m_size = Vector3D(data["Size"][0].GetFloat(), data["Size"][1].GetFloat(), data["Size"][2].GetFloat());
	}

	if (data.HasMember("BlendDistance"))
	{
		m_blendDistance = data["BlendDistance"].GetFloat();
	}
}

void ReflectionProbeComponent::DescribeProperties(PropertyBuilder<ReflectionProbeComponent>& properties)
{
	properties.Vector3("Size", &ReflectionProbeComponent::GetSize, &ReflectionProbeComponent::SetSize)
		.ToolTip("The box glossy surfaces reflect this probe in, metres along each world axis. Fit it to the room: reflections are projected onto its sides.");
	properties.Float("Blend Distance", &ReflectionProbeComponent::GetBlendDistance, &ReflectionProbeComponent::SetBlendDistance)
		.Range(0.0f, 10.0f).StepSize(0.05f).DisplayFormat("%.2f m")
		.ToolTip("How far inside the box's sides its reflection fades in");
}
