#pragma once

#include <Apollo/ECS/EntityComponent.h>

#include <Utils/Math/Vector3D.h>

struct LightSourceComponent : public Apollo::Component<LightSourceComponent>
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

	Vector3D Color;

	float Strength { 1.0f };
};