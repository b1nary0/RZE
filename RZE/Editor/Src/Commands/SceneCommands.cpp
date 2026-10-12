#include <Commands/SceneCommands.h>

#include <UI/Framework/EditorContext.h>

#include <RZE.h>

#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/TransformComponent.h>

#include <Game/SceneFormat.h>

#include <RapidJSON/document.h>

namespace
{
	// name, or "name (n)" with the lowest n that isn't in use. Cosmetic: names don't have to be unique, but
	// numbered copies are easier to tell apart in the Scene panel.
	std::string MakeUniqueName(const std::string& name)
	{
		GameScene& scene = RZE().GetActiveScene();
		if (scene.FindGameObjectByName(name) == nullptr)
		{
			return name;
		}

		// A copy of a copy is "Tank (2)", not "Tank (1) (1)"
		std::string base = name;
		const size_t suffixStart = name.rfind(" (");
		if (suffixStart != std::string::npos && name.back() == ')'
			&& name.find_first_not_of("0123456789", suffixStart + 2) == name.size() - 1)
		{
			base = name.substr(0, suffixStart);
		}

		for (int copyNumber = 1;; ++copyNumber)
		{
			const std::string candidate = base + " (" + std::to_string(copyNumber) + ")";
			if (scene.FindGameObjectByName(candidate) == nullptr)
			{
				return candidate;
			}
		}
	}

	// Moves gameObject by worldOffset in world space, whatever its parent's rotation and scale
	void MoveInWorld(const GameObjectPtr& gameObject, const Vector3D& worldOffset)
	{
		Vector3D localOffset = worldOffset;
		const GameObjectPtr parent = gameObject->GetParent();
		if (parent != nullptr)
		{
			// w = 0 makes it a direction, so only the parent's rotation and scale apply, not its position
			localOffset = (parent->GetTransformComponent()->GetWorldMatrix().Inverse() * Vector4D(worldOffset, 0.0f)).XYZ();
		}

		GameObjectComponentPtr<TransformComponent> transform = gameObject->GetTransformComponent();
		transform->SetPosition(transform->GetPosition() + localOffset);
	}
}

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

		// While the object is still alive, so SelectionBoundsDisplay can turn off its bounds
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

	PasteGameObjectsCommand::PasteGameObjectsCommand(std::string sceneData, const Vector3D& worldOffset)
		: m_sceneData(std::move(sceneData))
		, m_worldOffset(worldOffset)
	{
	}

	void PasteGameObjectsCommand::Execute(EditorContext& context)
	{
		rapidjson::Document document;
		document.Parse(m_sceneData.c_str());
		std::string error;
		if (document.HasParseError() || !SceneFormat::Upgrade(document, error))
		{
			return;
		}

		// Parents are linked by the GUIDs in the copied data, so the loader keeps copied children under their
		// pasted parents, and a copied object keeps its original parent if that's still in the scene
		GameObjectLoader loader(RZE().GetActiveScene());
		rapidjson::Document::AllocatorType& allocator = document.GetAllocator();
		for (rapidjson::Value& entry : SceneFormat::GetObjects(document).GetArray())
		{
			const std::string pastedName = MakeUniqueName(entry["name"].GetString());
			entry["name"].SetString(pastedName.c_str(), static_cast<rapidjson::SizeType>(pastedName.size()), allocator);
			loader.Load(entry);
		}

		for (const GameObjectPtr& gameObject : loader.GetTopLevelObjects())
		{
			MoveInWorld(gameObject, m_worldOffset);
		}

		if (!loader.GetTopLevelObjects().empty())
		{
			context.GetSelection().SetSelection(loader.GetTopLevelObjects());
		}
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
