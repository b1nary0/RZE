#include <StdAfx.h>
#include <Game/World/GameObjectComponents/TransformComponent.h>

TransformComponent::TransformComponent(const Vector3D& position)
{
	SetPosition(position);
}

TransformComponent::TransformComponent(const Vector3D& position, const Vector3D& rotation)
{
	SetPosition(position);
	SetRotation(rotation);
}

TransformComponent::TransformComponent(const Vector3D& position, const Vector3D& rotation, const Vector3D& scale)
{
	SetPosition(position);
	SetRotation(rotation);
	SetScale(scale);
}

void TransformComponent::SetPosition(const Vector3D& position)
{
	m_position = position;
}

void TransformComponent::SetRotation(const Vector3D& rotation)
{
	m_rotation = rotation;
}

void TransformComponent::SetScale(const Vector3D& scale)
{
	m_scale = scale;
}

Matrix4x4 TransformComponent::GetAsMat4x4() const
{
	return Matrix4x4::CreateInPlace(m_position, m_scale, m_rotation);
}

void TransformComponent::Rotate(const Vector3D& rotation)
{
	m_rotation = rotation;
}

const Vector3D& TransformComponent::GetPosition() const
{
	return m_position;
}

const Vector3D& TransformComponent::GetRotation() const
{
	return m_rotation;
}

const Vector3D& TransformComponent::GetScale() const
{
	return m_scale;
}

void TransformComponent::Save(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer)
{
}

void TransformComponent::Load(const rapidjson::Value& data)
{
}
