#pragma once

#include <Game/World/GameObject/GameObjectComponent.h>

#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector3D.h>

class TransformComponent : public GameObjectComponent<TransformComponent>
{
public:
	TransformComponent() = default;
	explicit TransformComponent(const Vector3D& position);
	TransformComponent(const Vector3D& position, const Vector3D& rotation);
	TransformComponent(const Vector3D& position, const Vector3D& rotation, const Vector3D& scale);

	// GameObjectComponent interface
public:
	static void DescribeProperties(PropertyBuilder<TransformComponent>& properties);

	void Serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) override;
	void Deserialize(const rapidjson::Value& data) override;

public:
	// Position in metres; rotation as Euler angles in degrees (see Utils/Math/Units.h)
	Vector3D& GetPosition();
	const Vector3D& GetPosition() const;
	Vector3D& GetRotation();
	const Vector3D& GetRotation() const;
	Vector3D& GetScale();
	const Vector3D& GetScale() const;

	void SetPosition(const Vector3D& position);
	void SetRotation(const Vector3D& rotation);
	void SetScale(const Vector3D& scale);

	// Helpers
	// Worked out when it's read after a change, here or in a parent, so it's right straight away (e.g. when a
	// render object is made for a mesh that's just been loaded), whether or not the scene has been updated since
	const Matrix4x4& GetWorldMatrix() const;
	// Where it sits in its parent (its position, rotation and scale), or in the world for a root. Worked out now,
	// rather than kept up to date each frame like GetWorldMatrix.
	Matrix4x4 GetLocalMatrix() const;

	//
	// Operations
	//
	void RotateBy(const Vector3D& rotation);

	void AttachTo(TransformComponent* parent);
	void DetachFromParent();


	bool IsRoot() const;

private:
	void MarkDirty();
	// Its position, rotation or scale changed. Moving within its parent changes the parent's hierarchy bounds
	// (GameObject::GetHierarchyBounds); a root moving changes no one's.
	void OnLocalTransformChanged();

	void AddChild(TransformComponent* child);
	void RemoveChild(TransformComponent* child);

private:
	// Whether m_worldTransform is out of date
	mutable bool m_isDirty = true;

	Vector3D m_position;
	Vector3D m_rotation;
	Vector3D	 m_scale { 1, 1, 1 };

	mutable Matrix4x4 m_worldTransform;

	TransformComponent* m_parent = nullptr;
	std::vector<TransformComponent*> m_children;
};
