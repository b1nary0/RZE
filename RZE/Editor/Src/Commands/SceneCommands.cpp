#include <Commands/SceneCommands.h>

#include <UI/Framework/EditorContext.h>

#include <RZE.h>

#include <Game/World/GameObject/GameObject.h>

namespace Editor
{
	CreateGameObjectCommand::CreateGameObjectCommand(std::string name)
		: m_name(std::move(name))
	{
	}

	void CreateGameObjectCommand::Execute(EditorContext& context)
	{
		RZE().GetActiveScene().AddGameObject(m_name);
	}

	RenameGameObjectCommand::RenameGameObjectCommand(GameObjectPtr gameObject, std::string name)
		: m_gameObject(gameObject)
		, m_name(std::move(name))
	{
	}

	void RenameGameObjectCommand::Execute(EditorContext& context)
	{
		if (m_gameObject != nullptr)
		{
			m_gameObject->SetName(m_name);
		}
	}

	DeleteGameObjectCommand::DeleteGameObjectCommand(GameObjectPtr gameObject)
		: m_gameObject(gameObject)
	{
	}

	void DeleteGameObjectCommand::Execute(EditorContext& context)
	{
		if (m_gameObject == nullptr)
		{
			return;
		}

		// While the object is still alive, so the selection can turn off its bounds
		context.GetSelection().Clear();

		// Deferred by the scene to its next update
		RZE().GetActiveScene().RemoveGameObject(m_gameObject);
	}

	AddComponentCommand::AddComponentCommand(GameObjectPtr gameObject, GameObjectComponentID componentID)
		: m_gameObject(gameObject)
		, m_componentID(componentID)
	{
	}

	void AddComponentCommand::Execute(EditorContext& context)
	{
		if (m_gameObject != nullptr)
		{
			m_gameObject->AddComponentByID(m_componentID);
		}
	}

	AttachToParentCommand::AttachToParentCommand(GameObjectPtr child, GameObjectPtr parent)
		: m_child(child)
		, m_parent(parent)
	{
	}

	void AttachToParentCommand::Execute(EditorContext& context)
	{
		if (m_child == nullptr || m_parent == nullptr || m_child->IsSelfOrAncestorOf(m_parent))
		{
			return;
		}

		if (!m_child->IsRoot())
		{
			m_child->DetachFromParent();
		}

		m_child->AttachTo(m_parent);
	}

	DetachFromParentCommand::DetachFromParentCommand(GameObjectPtr gameObject)
		: m_gameObject(gameObject)
	{
	}

	void DetachFromParentCommand::Execute(EditorContext& context)
	{
		if (m_gameObject != nullptr && !m_gameObject->IsRoot())
		{
			m_gameObject->DetachFromParent();
		}
	}
}
