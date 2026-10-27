#pragma once
#include <Game/World/GameObject/GameObjectComponent.h>

#include <Utils/Math/Vector3D.h>

// Swings its object back and forth through where it is, along Offset and back past it the other way, once each Period,
// so a moving object or light can be reproduced (e.g. to measure what moving things cost the renderer). It moves the
// object by how much the swing changed each Update, so the object can still be moved by hand while it swings, and
// turning it off puts the object back where the swing started from. Runs in the editor too.
//
// Saving the scene while it swings saves the object where it is in its swing, at most Offset from where it started.
class OscillatorComponent : public GameObjectComponent<OscillatorComponent>
{
public:
	OscillatorComponent() = default;
	~OscillatorComponent() override = default;

	void OnRemoveFromScene() override;

	void Update() override;

	void Serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) override;
	void Deserialize(const rapidjson::Value& data) override;

	static void DescribeProperties(PropertyBuilder<OscillatorComponent>& properties);

public:
	bool IsEnabled() const { return m_isEnabled; }
	void SetEnabled(bool isEnabled) { m_isEnabled = isEnabled; }

	// The furthest the swing goes from where it started, metres along each world axis
	const Vector3D& GetOffset() const { return m_offset; }
	void SetOffset(const Vector3D& offset) { m_offset = offset; }

	// Seconds for a whole swing, there and back both ways
	float GetPeriod() const { return m_period; }
	void SetPeriod(float period) { m_period = period; }

private:
	// Moves the object by the change from the swing's current offset to offset
	void MoveTo(const Vector3D& offset);

private:
	bool m_isEnabled = true;
	Vector3D m_offset = Vector3D(1.0f, 0.0f, 0.0f);
	float m_period = 4.0f;

	// Seconds into the swing
	double m_time = 0.0;
	// How far the object has been moved from where the swing started
	Vector3D m_appliedOffset;
};
