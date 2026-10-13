#include <Commands/TransformCommands.h>

#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/TransformComponent.h>

namespace
{
	struct TransformValues
	{
		Vector3D Position;
		Vector3D Rotation;
		Vector3D Scale;
	};

	class SetTransformRecord : public Editor::UndoRecord
	{
	public:
		SetTransformRecord(ObjectGUID guid, const TransformValues& before, const TransformValues& after)
			: m_guid(guid)
			, m_before(before)
			, m_after(after)
		{
		}

		void Undo(Editor::EditorContext& context) override { Apply(m_before); }
		void Redo(Editor::EditorContext& context) override { Apply(m_after); }
		std::string GetName() const override { return "Change Transform"; }

		bool TryMerge(const Editor::UndoRecord& newer) override
		{
			const SetTransformRecord* const newerTransform = dynamic_cast<const SetTransformRecord*>(&newer);
			if (newerTransform == nullptr || newerTransform->m_guid != m_guid)
			{
				return false;
			}

			m_after = newerTransform->m_after;
			return true;
		}

	private:
		void Apply(const TransformValues& values)
		{
			const GameObjectPtr gameObject = Editor::UndoTargets::FindObject(m_guid);
			if (gameObject == nullptr)
			{
				return;
			}

			GameObjectComponentPtr<TransformComponent> transform = gameObject->GetTransformComponent();
			transform->SetPosition(values.Position);
			transform->SetRotation(values.Rotation);
			transform->SetScale(values.Scale);
		}

	private:
		ObjectGUID m_guid;
		TransformValues m_before;
		TransformValues m_after;
	};
}

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

	bool SetTransformCommand::Execute(EditorContext& context)
	{
		if (m_gameObject == nullptr)
		{
			return false;
		}

		GameObjectComponentPtr<TransformComponent> transform = m_gameObject->GetTransformComponent();
		const TransformValues before{ transform->GetPosition(), transform->GetRotation(), transform->GetScale() };
		const TransformValues after{ m_position, m_rotation, m_scale };
		SetUndoRecord(std::make_unique<SetTransformRecord>(m_gameObject->GetGUID(), before, after));

		transform->SetPosition(m_position);
		transform->SetRotation(m_rotation);
		transform->SetScale(m_scale);
		return true;
	}
}
