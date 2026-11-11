#pragma once
#include <Game/World/GameObject/GameObjectComponent.h>

#include <Graphics/GraphicsDefines.h>

#include <Utils/Math/Vector3D.h>

class RenderScene;

// A reflection probe (ReflectionProbeObject): captures the scene around the object's position, and glossy surfaces in
// its box, centred there and aligned to the world's axes, reflect it. Size the box to the room it's in, since the
// reflections are projected onto its sides.
class ReflectionProbeComponent final : public GameObjectComponent<ReflectionProbeComponent>
{
public:
	ReflectionProbeComponent() = default;
	~ReflectionProbeComponent() override = default;

	void Initialize() override;
	void OnAddToScene() override;
	void OnRemoveFromScene() override;

	void Update() override;

	void Serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) override;
	void Deserialize(const rapidjson::Value& data) override;

	static void DescribeProperties(PropertyBuilder<ReflectionProbeComponent>& properties);

public:
	// The box's size along each world axis, metres. The probe picks it up on its next Update.
	const Vector3D& GetSize() const { return m_size; }
	void SetSize(const Vector3D& size) { m_size = size; }

	// How far inside the box's sides its reflection fades in, metres
	float GetBlendDistance() const { return m_blendDistance; }
	void SetBlendDistance(float blendDistance) { m_blendDistance = blendDistance; }

private:
	// The owner's scene's; set in Initialize, once the component has an owner, and fixed from then on since objects
	// never change scene
	RenderScene* m_renderScene = nullptr;
	ReflectionProbeObjectPtr m_reflectionProbe;

	Vector3D m_size = Vector3D(8.0f, 4.0f, 8.0f);
	float m_blendDistance = 0.5f;
};
