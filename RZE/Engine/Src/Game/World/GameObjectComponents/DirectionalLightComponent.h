#pragma once
#include <Game/World/GameObject/GameObjectComponent.h>

#include <Graphics/RenderEngine.h>

#include <Utils/Math/Vector3D.h>

class DirectionalLightComponent : public GameObjectComponent<DirectionalLightComponent>
{
public:
	DirectionalLightComponent() = default;
	~DirectionalLightComponent() override = default;

	void OnAddToScene() override;
	void OnRemoveFromScene() override;

	void Update() override;

	void Serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) override;
	void Deserialize(const rapidjson::Value& data) override;

	static void DescribeProperties(PropertyBuilder<DirectionalLightComponent>& properties);

	void OnEditorInspect() override;

public:
	// RGBA, each in [0, 1]; the light picks it up on its next Update
	const Vector4D& GetLightColour() const { return m_lightColour; }
	void SetLightColour(const Vector4D& colour) { m_lightColour = colour; }

	float GetLightStrength() const { return m_lightStrength; }
	void SetLightStrength(float strength) { m_lightStrength = strength; }

private:
	LightObjectPtr m_lightObject;

	float m_lightStrength = 2.0f;
	Vector4D m_lightColour = Vector4D(1.0f);
};