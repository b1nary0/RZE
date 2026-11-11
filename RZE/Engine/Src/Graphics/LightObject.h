#pragma once

#include <Utils/Math/Vector3D.h>
#include <Utils/Math/Vector4D.h>
#include <Utils/PrimitiveDefs.h>

// How a light shines, which decides how it's lit and shadowed. The shaders branch on the values (Light::Type in
// Common/PixelResources.hlsli), so new types go at the end.
enum class ELightType
{
	// From one direction everywhere, like the sun; position doesn't matter
	Directional,
	// From a point in every direction, fading out by its range
	Point
};

// A light in a scene: the data every type shares, read by its type. It has no GPU resources of its own:
// ShadowRenderStage puts all of a scene's lights into one buffer.
class LightObject
{
	// Hands out m_sceneRevision
	friend class RenderScene;

public:
	// Alive until destroyed (RenderSceneLifetime)
	explicit LightObject(ELightType type);
	~LightObject();

	LightObject(const LightObject&) = delete;
	LightObject& operator=(const LightObject&) = delete;

	// Never handed out twice, to a light or a render object, so a LightObjectPtr can tell once this light is gone
	U64 GetLiveID() const { return m_liveID; }

	// The setters below mark the light's shadows changed when the value moves them (see MarkShadowsChanged)
	void SetType(ELightType type);
	ELightType GetType() const { return m_type; }

	// World space; only used by point lights
	void SetPosition(const Vector3D& position);
	const Vector3D& GetPosition() const { return m_position; }

	// World space, normalized; the direction light travels. Only used by directional lights.
	void SetDirection(const Vector3D& direction);
	const Vector3D& GetDirection() const { return m_direction; }

	// Distance in metres at which a point light has faded to nothing; only used by point lights
	void SetRange(float range);
	float GetRange() const { return m_range; }

	// The fraction of its strength a point light gives at rangeRatio (distance / range): 1 at the light, falling to 0
	// at its range. Mirrors LightAttenuation in Common/Lighting.hlsli.
	static float CalculatePointFalloff(float rangeRatio);
	// The rangeRatio within which a point light of this strength and colour (sRGB) lights a surface facing it more
	// than the ambient light does, so it's the main light there; 0 if it never does. Grows with strength, up to 1.
	static float CalculateOutshinesAmbientRatio(float strength, const Vector4D& colour);

	void SetStrength(float strength) { m_strength = strength; }
	float GetStrength() const { return m_strength; }

	// sRGB
	void SetColour(const Vector4D& colour) { m_colour = colour; }
	const Vector4D& GetColour() const { return m_colour; }

	// RenderScene::GetRevision when the light was created or its shadows last moved, so a shadow map rendered at an
	// older revision is out of date. 0 for a light in no scene.
	U64 GetShadowRevision() const { return m_shadowRevision; }

private:
	// For changes that move the light's shadows, so its shadow map is made again: a new scene revision, which becomes
	// the light's shadow revision. Colour and strength don't move them.
	void MarkShadowsChanged();

private:
	ELightType m_type;
	Vector3D m_position;
	Vector3D m_direction = Vector3D(0.0f, -1.0f, 0.0f);
	Vector4D m_colour = Vector4D(1.0f);
	float m_strength = 1.0f;
	float m_range = 10.0f;

	// RenderScene::GetRevision of the scene this light is in
	U64* m_sceneRevision = nullptr;
	U64 m_shadowRevision = 0;

	const U64 m_liveID;
};
