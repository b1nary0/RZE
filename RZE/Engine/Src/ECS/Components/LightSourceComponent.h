#pragma once

#include <Apollo/ECS/EntityComponent.h>

#include <Utils/Math/Vector3D.h>

enum class ELightType
{
	Directional = 0,
	Point
};

struct LightSourceComponent final : public Apollo::Component<LightSourceComponent>
{
	LightSourceComponent()
		: Color(Vector3D(1.0f, 1.0f, 1.0f))
		, Strength(1.0f)
		, LightType(ELightType::Directional)
	{}

	LightSourceComponent(const Vector3D& color)
		: Color(color)
		, Strength(1.0f)
		, LightType(ELightType::Directional)
	{}

	LightSourceComponent(const Vector3D& color, float strength)
		: Color(color)
		, Strength(strength)
		, LightType(ELightType::Directional)
	{}

	LightSourceComponent(const Vector3D& color, float strength, ELightType lightType)
		: Color(color)
		, Strength(strength)
		, LightType(lightType)
	{}

	LightSourceComponent(ELightType lightType, const Vector3D& color, float strength)
		: LightType(lightType)
		, Color(color)
		, Strength(strength)
	{}

	virtual void OnEditorInspect(Apollo::EntityID entityID) override;

	virtual void Save(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer);
	virtual void Load(const rapidjson::Value& data) override;

	ELightType LightType;
	Vector3D Color;
	float Strength { 1.0f };
};