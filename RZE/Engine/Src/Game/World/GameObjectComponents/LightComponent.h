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

private:
	// The owner's scene's; set in Initialize, once the component has an owner, and fixed from then on since objects
	// never change scene
	RenderScene* m_renderScene = nullptr;
	LightObjectPtr m_lightObject;

	ELightType m_lightType = ELightType::Point;
	float m_lightStrength = 5.0f;
	float m_range = 10.0f;
	Vector4D m_lightColour = Vector4D(1.0f);
};
