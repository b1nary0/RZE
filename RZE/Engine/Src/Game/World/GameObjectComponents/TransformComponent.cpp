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

void TransformComponent::OnEditorInspect()
{
	float* positionValues = const_cast<float*>(&m_position.GetInternalVec().x);
	float* rotationValues = const_cast<float*>(&m_rotation.GetInternalVec().x);
	float* scaleValues = const_cast<float*>(&m_scale.GetInternalVec().x);

	// #TODO(Modify DragFloat3 to only register on mouse movement to avoid falloff when dragging)

	ImGui::Text("Position");
	ImGui::DragFloat3("##transfcomp_position", positionValues, 0.005f);
	ImGui::Text("Rotation");
	ImGui::DragFloat3("##transfcomp_rotation", rotationValues, 0.05f);
	ImGui::Text("Scale");
	ImGui::DragFloat3("##transfcomp_scale", scaleValues, 0.005f);
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
	writer.String("TransformComponent");
	writer.StartObject();
	{
		{
			writer.Key("Position");
			writer.StartArray();
			for (int i = 0; i < 3; ++i)
			{
				writer.Double(m_position[i]);
			}
			writer.EndArray();
		}
		{
			writer.Key("Scale");
			writer.StartArray();
			for (int i = 0; i < 3; ++i)
			{
				writer.Double(m_scale[i]);
			}
			writer.EndArray();
		}
		{
			writer.Key("Rotation");
			writer.StartArray();
			for (int i = 0; i < 3; ++i)
			{
				writer.Double(m_rotation[i]);
			}
			writer.EndArray();
		}
	}
	writer.EndObject();
}

void TransformComponent::Load(const rapidjson::Value& data)
{
	m_position = Vector3D(data["Position"][0].GetFloat(), data["Position"][1].GetFloat(), data["Position"][2].GetFloat());
	m_scale = Vector3D(data["Scale"][0].GetFloat(), data["Scale"][1].GetFloat(), data["Scale"][2].GetFloat());
	m_rotation = Vector3D(data["Rotation"][0].GetFloat(), data["Rotation"][1].GetFloat(), data["Rotation"][2].GetFloat());
}
