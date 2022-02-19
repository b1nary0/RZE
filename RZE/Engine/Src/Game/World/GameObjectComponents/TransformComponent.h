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
	void OnEditorInspect() override;
	void Save(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) override;
	void Load(const rapidjson::Value& data) override;

public:
	const Vector3D& GetPosition() const;
	const Vector3D& GetRotation() const;
	const Vector3D& GetScale() const;

	void SetPosition(const Vector3D& position);
	void SetRotation(const Vector3D& rotation);
	void SetScale(const Vector3D& scale);

	// Helpers
	Matrix4x4 GetAsMat4x4() const;

	//
	// Operations
	//
	void Rotate(const Vector3D& rotation);

private:
	Vector3D m_position;
	Vector3D	 m_rotation;
	Vector3D	 m_scale { 1, 1, 1 };
};