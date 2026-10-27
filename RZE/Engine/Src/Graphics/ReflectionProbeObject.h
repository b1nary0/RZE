#pragma once

#include <Utils/Math/BoundingBox.h>
#include <Utils/Math/Vector3D.h>
#include <Utils/PrimitiveDefs.h>

// A reflection probe in a scene: a point that captures its surroundings for glossy surfaces to reflect
// (ReflectionProbeStage), and the box it's used in. Surfaces in the box reflect what the probe saw, corrected for
// where in the box they are (box-projected, as if the box were the room's walls), fading out over the blend distance
// inside its sides. It has no GPU resources of its own: ReflectionProbeStage keeps a scene's probes together.
class ReflectionProbeObject
{
public:
	// sceneRevision: RenderScene::GetRevision's counter, which moving or resizing the probe advances, so its captures
	// can tell they're out of date. It must outlive the probe.
	explicit ReflectionProbeObject(U64& sceneRevision);
	~ReflectionProbeObject();

	ReflectionProbeObject(const ReflectionProbeObject&) = delete;
	ReflectionProbeObject& operator=(const ReflectionProbeObject&) = delete;

	// Never handed out twice (RenderSceneLifetime), so a ReflectionProbeObjectPtr can tell once this probe is gone
	U64 GetLiveID() const { return m_liveID; }

	// World space: where it captures from, and the centre of its box
	void SetPosition(const Vector3D& position);
	const Vector3D& GetPosition() const { return m_position; }

	// Half the box's size along each world axis, metres
	void SetExtents(const Vector3D& extents);
	const Vector3D& GetExtents() const { return m_extents; }

	// How far inside the box's sides its reflection fades in, metres
	void SetBlendDistance(float blendDistance) { m_blendDistance = blendDistance; }
	float GetBlendDistance() const { return m_blendDistance; }

	BoundingBox GetBox() const { return BoundingBox(m_position - m_extents, m_position + m_extents); }

	// RenderScene::GetRevision when the probe was made, moved or resized
	U64 GetRevision() const { return m_revision; }

private:
	void MarkChanged();

private:
	Vector3D m_position;
	Vector3D m_extents = Vector3D(4.0f, 2.0f, 4.0f);
	float m_blendDistance = 0.5f;

	U64& m_sceneRevision;
	U64 m_revision = 0;

	const U64 m_liveID;
};
