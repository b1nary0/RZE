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
	{}

	LightSourceComponent(const Vector3D& color)
		: Color(color)
	{}

	LightSourceComponent(const Vector3D& color, float strength)
		: Color(color)
		, Strength(strength)
	{}

	LightSourceComponent(ELightType lightType, const Vector3D& color, float strength)
		: LightType(lightType)
		, Color(color)
		, Strength(strength)
	{}

	ELightType LightType;
	Vector3D Color;
	float Strength { 1.0f };
};