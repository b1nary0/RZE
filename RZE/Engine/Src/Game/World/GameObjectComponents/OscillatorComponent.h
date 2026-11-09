#pragma once

#include <Utils/DebugUtils/DebugConfig.h>

// A development aid, for stress and test scenes: Retail doesn't have it, and a scene using it loads there without it
#ifdef RZE_DEVEL

#include <Game/World/GameObject/GameObjectComponent.h>

#include <Utils/Math/Vector3D.h>

// Moves its object back and forth along Axis around Origin while it's on: a sine wave Amplitude metres either side,
// one cycle every Period seconds, starting Phase of a cycle in. Where the object is comes from the time alone, so it
// never drifts. While on, it owns the object's position: a gizmo drag or a Transform edit is undone the next frame.
// Move an oscillating object by changing its Origin, or turn it off, which puts the object back at its Origin; turning
// it on makes wherever the object is then its Origin.
class OscillatorComponent final : public GameObjectComponent<OscillatorComponent>
{
public:
	static void DescribeProperties(PropertyBuilder<OscillatorComponent>& properties);

	// Takes the object's position as its Origin, for a component added to an object rather than loaded
	void Initialize() override;
	void Update() override;

	void Serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) override;
	void Deserialize(const rapidjson::Value& data) override;

public:
	// Moves its clock on by seconds and, if it's on, puts the object where the motion is then. Update calls it with
	// the frame's time.
	void Advance(double seconds);
	// How far from Origin the object is at seconds into the motion; nothing if it can't move (no amplitude, a zero
	// axis or period)
	Vector3D GetOffsetAt(double seconds) const;

	bool IsEnabled() const { return m_isEnabled; }
	// Off puts the object back at Origin; on makes its position the Origin
	void SetEnabled(bool isEnabled);

	// Which way it moves; any length, as only the direction is used
	const Vector3D& GetAxis() const { return m_axis; }
	void SetAxis(const Vector3D& axis) { m_axis = axis; }

	// Metres either side of Origin
	float GetAmplitude() const { return m_amplitude; }
	void SetAmplitude(float amplitude) { m_amplitude = amplitude; }

	// Seconds for a whole cycle
	float GetPeriod() const { return m_period; }
	void SetPeriod(float period) { m_period = period; }

	// How far into a cycle the motion starts, in cycles: 0.25 starts at the top of the swing
	float GetPhase() const { return m_phase; }
	void SetPhase(float phase) { m_phase = phase; }

	// The point it moves around, where the object's position is while it's off
	const Vector3D& GetOrigin() const { return m_origin; }
	void SetOrigin(const Vector3D& origin) { m_origin = origin; }

private:
	bool m_isEnabled = true;
	Vector3D m_axis = Vector3D(0.0f, 1.0f, 0.0f);
	float m_amplitude = 1.0f;
	float m_period = 2.0f;
	float m_phase = 0.0f;
	Vector3D m_origin;

	// Seconds into the current cycle, kept within one period so precision doesn't wear away over a long run
	double m_time = 0.0;
};

#endif
