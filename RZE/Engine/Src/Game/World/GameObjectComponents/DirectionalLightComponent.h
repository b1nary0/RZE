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

	void OnEditorInspect() override;

private:
	LightObjectPtr m_lightObject;

	float m_lightStrength = 1.0f;
	Vector4D m_lightColour = Vector4D(1.0f);
};