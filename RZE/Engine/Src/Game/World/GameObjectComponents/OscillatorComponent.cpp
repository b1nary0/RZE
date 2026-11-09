#include <Game/World/GameObjectComponents/OscillatorComponent.h>

#ifdef RZE_DEVEL

#include <RZE.h>

#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/TransformComponent.h>

#include <Utils/Math/Math.h>

#include <cmath>

namespace
{
	void WriteVector(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer, const char* key, const Vector3D& vector)
	{
		writer.Key(key);
		writer.StartArray();
		for (int i = 0; i < 3; ++i)
		{
			writer.Double(vector[i]);
		}
		writer.EndArray();
	}

	// Leaves vector as it is if data hasn't got key
	void ReadVector(const rapidjson::Value& data, const char* key, Vector3D& vector)
	{
		if (data.HasMember(key))
		{
			const rapidjson::Value& values = data[key];
			vector = Vector3D(values[0].GetFloat(), values[1].GetFloat(), values[2].GetFloat());
		}
	}

	void ReadFloat(const rapidjson::Value& data, const char* key, float& value)
	{
		if (data.HasMember(key))
		{
			value = data[key].GetFloat();
		}
	}
}

void OscillatorComponent::Initialize()
{
	// A loaded component's Deserialize, which comes next, sets its own
	GameObjectComponentPtr<TransformComponent> transform = GetOwner()->GetComponent<TransformComponent>();
	if (transform != nullptr)
	{
		m_origin = transform->GetPosition();
	}
}

void OscillatorComponent::Update()
{
	Advance(RZE().GetDeltaTime());
}

void OscillatorComponent::Advance(double seconds)
{
	if (!m_isEnabled)
	{
		return;
	}

	if (m_period > 0.0f)
	{
		m_time = std::fmod(m_time + seconds, static_cast<double>(m_period));
	}

	GameObjectComponentPtr<TransformComponent> transform = GetOwner()->GetComponent<TransformComponent>();
	if (transform != nullptr)
	{
		transform->SetPosition(m_origin + GetOffsetAt(m_time));
	}
}

Vector3D OscillatorComponent::GetOffsetAt(double seconds) const
{
	const float axisLength = m_axis.Length();
	if (m_period <= 0.0f || axisLength < VectorUtils::kEpsilon)
	{
		return Vector3D();
	}

	const double cycles = seconds / static_cast<double>(m_period) + static_cast<double>(m_phase);
	const float wave = static_cast<float>(std::sin(2.0 * static_cast<double>(MathUtils::Pi) * cycles));
	return m_axis * (m_amplitude * wave / axisLength);
}

void OscillatorComponent::SetEnabled(bool isEnabled)
{
	if (isEnabled == m_isEnabled)
	{
		return;
	}
	m_isEnabled = isEnabled;

	// Not on an object yet: there's no position to take or put back
	if (GetOwner() == nullptr)
	{
		return;
	}
	GameObjectComponentPtr<TransformComponent> transform = GetOwner()->GetComponent<TransformComponent>();
	if (transform == nullptr)
	{
		return;
	}

	if (isEnabled)
	{
		m_origin = transform->GetPosition();
	}
	else
	{
		transform->SetPosition(m_origin);
	}
}

void OscillatorComponent::Serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer)
{
	writer.String("OscillatorComponent");
	writer.StartObject();
	{
		writer.Key("Enabled");
		writer.Bool(m_isEnabled);
		WriteVector(writer, "Axis", m_axis);
		writer.Key("Amplitude");
		writer.Double(m_amplitude);
		writer.Key("Period");
		writer.Double(m_period);
		writer.Key("Phase");
		writer.Double(m_phase);
		WriteVector(writer, "Origin", m_origin);
	}
	writer.EndObject();
}

void OscillatorComponent::Deserialize(const rapidjson::Value& data)
{
	// Whatever's missing stays as a new component has it; the Origin, as Initialize took it from the position
	if (data.HasMember("Enabled"))
	{
		m_isEnabled = data["Enabled"].GetBool();
	}
	ReadVector(data, "Axis", m_axis);
	ReadFloat(data, "Amplitude", m_amplitude);
	ReadFloat(data, "Period", m_period);
	ReadFloat(data, "Phase", m_phase);
	ReadVector(data, "Origin", m_origin);
}

void OscillatorComponent::DescribeProperties(PropertyBuilder<OscillatorComponent>& properties)
{
	properties.Bool("Enabled", &OscillatorComponent::IsEnabled, &OscillatorComponent::SetEnabled)
		.ToolTip("Off puts the object back at its Origin; on makes where it is its Origin");
	properties.Vector3("Axis", &OscillatorComponent::GetAxis, &OscillatorComponent::SetAxis)
		.ToolTip("Which way it moves; only the direction counts");
	properties.Float("Amplitude", &OscillatorComponent::GetAmplitude, &OscillatorComponent::SetAmplitude)
		.Range(0.0f, 100.0f).StepSize(0.05f).DisplayFormat("%.2f m");
	properties.Float("Period", &OscillatorComponent::GetPeriod, &OscillatorComponent::SetPeriod)
		.Range(0.05f, 60.0f).StepSize(0.05f).DisplayFormat("%.2f s");
	properties.Float("Phase", &OscillatorComponent::GetPhase, &OscillatorComponent::SetPhase)
		.Range(0.0f, 1.0f).StepSize(0.01f).DisplayFormat("%.2f cycles");
	properties.Vector3("Origin", &OscillatorComponent::GetOrigin, &OscillatorComponent::SetOrigin)
		.ToolTip("Where it moves around. While it's on, move the object by changing this.")
		.EnabledIf<OscillatorComponent>([](const OscillatorComponent& oscillator) { return oscillator.IsEnabled(); },
			"Off, the object's own position is used; turning it on takes that as the Origin");
}

#endif
