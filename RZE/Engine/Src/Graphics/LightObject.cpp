#include <Graphics/LightObject.h>

#include <Graphics/GraphicsDefines.h>

#include <algorithm>
#include <cmath>

namespace
{
	// Mirrors PointFalloffSharpness in Common/Lighting.hlsli
	constexpr float k_pointFalloffSharpness = 8.0f;
	// Mirror AmbientSky and AmbientGround in Common/Lighting.hlsli; linear
	constexpr float k_ambientSky[3] = { 0.14f, 0.16f, 0.20f };
	constexpr float k_ambientGround[3] = { 0.05f, 0.045f, 0.04f };

	// Rec. 709 weights, for linear colours
	float Luminance(float r, float g, float b)
	{
		return 0.2126f * r + 0.7152f * g + 0.0722f * b;
	}

	// As SRGBToLinear in Common/ColourSpace.hlsli
	float SRGBToLinear(float value)
	{
		return std::pow(std::max(value, 0.0f), 2.2f);
	}
}

LightObject::LightObject(ELightType type)
	: m_type(type)
	, m_liveID(RenderSceneLifetime::Acquire())
{
}

LightObject::~LightObject()
{
	RenderSceneLifetime::Release(m_liveID);
}

void LightObject::SetType(ELightType type)
{
	if (type != m_type)
	{
		m_type = type;
		MarkShadowsChanged();
	}
}

void LightObject::SetPosition(const Vector3D& position)
{
	if (position != m_position)
	{
		m_position = position;
		if (m_type == ELightType::Point)
		{
			MarkShadowsChanged();
		}
	}
}

void LightObject::SetDirection(const Vector3D& direction)
{
	if (direction != m_direction)
	{
		m_direction = direction;
		if (m_type == ELightType::Directional)
		{
			MarkShadowsChanged();
		}
	}
}

float LightObject::CalculatePointFalloff(float rangeRatio)
{
	const float rangeRatioSq = rangeRatio * rangeRatio;
	const float window = std::clamp(1.0f - rangeRatioSq * rangeRatioSq, 0.0f, 1.0f);
	return window * window / (1.0f + k_pointFalloffSharpness * rangeRatioSq);
}

float LightObject::CalculateOutshinesAmbientRatio(float strength, const Vector4D& colour)
{
	// What a wall gets: halfway between the sky above and the ground below (HemisphereAmbient)
	const float ambient = Luminance(
		(k_ambientSky[0] + k_ambientGround[0]) * 0.5f,
		(k_ambientSky[1] + k_ambientGround[1]) * 0.5f,
		(k_ambientSky[2] + k_ambientGround[2]) * 0.5f);
	// Right next to the light, before falloff (LightRadiance)
	const float light = strength * Luminance(SRGBToLinear(colour.X()), SRGBToLinear(colour.Y()), SRGBToLinear(colour.Z()));

	if (light <= ambient)
	{
		return 0.0f;
	}

	// The falloff only goes down between the light and its range, so halving the interval homes in on the crossing
	float low = 0.0f;
	float high = 1.0f;
	for (int step = 0; step < 24; ++step)
	{
		const float middle = (low + high) * 0.5f;
		(light * CalculatePointFalloff(middle) > ambient ? low : high) = middle;
	}
	return (low + high) * 0.5f;
}

void LightObject::SetRange(float range)
{
	if (range != m_range)
	{
		m_range = range;
		if (m_type == ELightType::Point)
		{
			MarkShadowsChanged();
		}
	}
}

void LightObject::MarkShadowsChanged()
{
	if (m_sceneRevision != nullptr)
	{
		m_shadowRevision = ++*m_sceneRevision;
	}
}
