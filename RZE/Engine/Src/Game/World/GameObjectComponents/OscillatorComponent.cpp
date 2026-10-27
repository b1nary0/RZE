#include <StdAfx.h>
#include <Game/World/GameObjectComponents/OscillatorComponent.h>

#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/TransformComponent.h>

#include <Utils/Math/Math.h>

#include <cmath>

void OscillatorComponent::OnRemoveFromScene()
{
	// Back where the swing started, e.g. when the component is removed
	MoveTo(Vector3D());
	m_time = 0.0;
}

void OscillatorComponent::Update()
{
	if (!m_isEnabled || m_period <= 0.0f)
	{
		MoveTo(Vector3D());
		m_time = 0.0;
		return;
	}

	m_time = std::fmod(m_time + RZE().GetDeltaTime(), static_cast<double>(m_period));
	const float phase = static_cast<float>(m_time / m_period) * 2.0f * MathUtils::Pi;
	MoveTo(m_offset * std::sin(phase));
}

void OscillatorComponent::MoveTo(const Vector3D& offset)
{
	if (offset == m_appliedOffset)
	{
		return;
	}

	GameObjectComponentPtr<TransformComponent> transformComponent = GetOwner()->GetTransformComponent();
	transformComponent->SetPosition(transformComponent->GetPosition() + offset - m_appliedOffset);
	m_appliedOffset = offset;
}

void OscillatorComponent::Serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer)
{
	writer.String("OscillatorComponent");
	writer.StartObject();
	{
		writer.Key("Enabled");
		writer.Bool(m_isEnabled);

		writer.Key("Offset");
		writer.StartArray();
		{
			for (int i = 0; i < 3; ++i)
			{
				writer.Double(m_offset[i]);
			}
		}
		writer.EndArray();

		writer.Key("Period");
		writer.Double(m_period);
	}
	writer.EndObject();
}

void OscillatorComponent::Deserialize(const rapidjson::Value& data)
{
	if (data.HasMember("Enabled"))
	{
		m_isEnabled = data["Enabled"].GetBool();
	}

	if (data.HasMember("Offset"))
	{
		m_offset = Vector3D(data["Offset"][0].GetFloat(), data["Offset"][1].GetFloat(), data["Offset"][2].GetFloat());
	}

	if (data.HasMember("Period"))
	{
		m_period = data["Period"].GetFloat();
	}
}

void OscillatorComponent::DescribeProperties(PropertyBuilder<OscillatorComponent>& properties)
{
	properties.Bool("Enabled", &OscillatorComponent::IsEnabled, &OscillatorComponent::SetEnabled)
		.ToolTip("Turning it off puts the object back where the swing started");
	properties.Vector3("Offset", &OscillatorComponent::GetOffset, &OscillatorComponent::SetOffset)
		.DisplayFormat("%.2f m").ToolTip("The furthest the swing goes from where it started, along each world axis; it goes as far the other way too");
	properties.Float("Period", &OscillatorComponent::GetPeriod, &OscillatorComponent::SetPeriod)
		.Range(0.1f, 60.0f).StepSize(0.05f).DisplayFormat("%.2f s")
		.ToolTip("Seconds for a whole swing, there and back");
}
