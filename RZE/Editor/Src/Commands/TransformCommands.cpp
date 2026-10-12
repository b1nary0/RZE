#include <Commands/TransformCommands.h>

#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/TransformComponent.h>

namespace Editor
{
	SetTransformCommand::SetTransformCommand(GameObjectPtr gameObject, const Vector3D& position, const Vector3D& rotation, const Vector3D& scale, InteractionID interaction)
		: EditorCommand(interaction)
		, m_gameObject(gameObject)
		, m_position(position)
		, m_rotation(rotation)
		, m_scale(scale)
	{
	}

	void SetTransformCommand::Execute(EditorContext& context)
	{
		if (m_gameObject == nullptr)
		{
			return;
		}

		GameObjectComponentPtr<TransformComponent> transform = m_gameObject->GetTransformComponent();
		transform->SetPosition(m_position);
		transform->SetRotation(m_rotation);
		transform->SetScale(m_scale);
	}
}
