#pragma once
#include <Game/World/GameObject/GameObjectComponent.h>

#include <Graphics/GraphicsDefines.h>
#include <Graphics/LightObject.h>

#include <Utils/Math/Vector4D.h>

class RenderScene;

// A light of any type (ELightType), picked from a drop-down; a new one is a point light. A directional light shines
// along the object's local -Y axis, so a zero rotation is a straight-down sun; a point light shines from the object's
// position.
class LightComponent final : public GameObjectComponent<LightComponent>
{
public:
	LightComponent() = default;
	~LightComponent() override = default;

	void Initialize() override;
	void OnAddToScene() override;
	void OnRemoveFromScene() override;

	void Update() override;

	void Serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) override;
	void Deserialize(const rapidjson::Value& data) override;

	static void DescribeProperties(PropertyBuilder<LightComponent>& properties);


public:
	// The light picks these up on its next Update
	ELightType GetLightType() const { return m_lightType; }
	void SetLightType(ELightType type) { m_lightType = type; }

	// RGBA, each in [0, 1]
	const Vector4D& GetLightColour() const { return m_lightColour; }
	void SetLightColour(const Vector4D& colour) { m_lightColour = colour; }

	float GetLightStrength() const { return m_lightStrength; }
	void SetLightStrength(float strength) { m_lightStrength = strength; }

	// Metres; only used by point lights
	float GetRange() const { return m_range; }
	void SetRange(float range) { m_range = range; }

	// How much the strength flickers, like a flame: 0 for steady, up to 1 for dipping all the way to dark. The strength
	// itself stays as set; only the light's is varied.
	float GetFlicker() const { return m_flicker; }
	void SetFlicker(float flicker) { m_flicker = flicker; }

private:
	// The strength to light with now: the set strength, dipped by the flicker
	float CalculateFlickeringStrength() const;

private:
	// The owner's scene's; set in Initialize, once the component has an owner, and fixed from then on since objects
	// never change scene
	RenderScene* m_renderScene = nullptr;
	LightObjectPtr m_lightObject;

	ELightType m_lightType = ELightType::Point;
	float m_lightStrength = 5.0f;
	float m_range = 10.0f;
	Vector4D m_lightColour = Vector4D(1.0f);
	float m_flicker = 0.0f;
	// Seconds the light has flickered for, wrapped so it stays precise
	double m_flickerTime = 0.0;
};
