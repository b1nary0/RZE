#pragma once

#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector3D.h>

struct TransformComponent
{
	TransformComponent() = default;
	
	explicit TransformComponent(const Vector3D& position)
	{
		SetPosition(position);
	}

	TransformComponent(const Vector3D& position, const Vector3D& rotation)
	{
		SetPosition(position);
		SetRotation(rotation);
	}

	TransformComponent(const Vector3D& position, const Vector3D& rotation, const Vector3D& scale)
	{
		SetPosition(position);
		SetRotation(rotation);
		SetScale(scale);
	}

	//
	// Direct Setters
	//
	void SetPosition(const Vector3D& position)
	{
		m_position = position;
	}

	void SetRotation(const Vector3D& rotation)
	{
		Rotation = rotation;
	}

	void SetScale(const Vector3D& scale)
	{
		Scale = scale;
	}

	// Helpers
	Matrix4x4 GetAsMat4x4() const
	{
		return Matrix4x4::CreateInPlace(m_position, Scale, Rotation);
	}

	//
	// Operations
	//
	void Rotate(const Vector3D& rotation)
	{
		Rotation += rotation;
	}

	//virtual void OnEditorInspect(Apollo::EntityID entityID) override;
	//virtual void Save(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer);
	//virtual void Load(const rapidjson::Value& data) override;

	Vector3D	m_position;
 	Vector3D	Scale {1, 1, 1};
	Vector3D	Rotation;
};