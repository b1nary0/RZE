#include <Commands/SceneCommands.h>

#include <UI/Framework/EditorContext.h>

#include <RZE.h>

#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/CameraComponent.h>
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

	class RenameRecord : public Editor::UndoRecord
	{
	public:
		RenameRecord(ObjectGUID guid, std::string before, std::string after)
			: m_guid(guid)
			, m_before(std::move(before))
			, m_after(std::move(after))
		{
		}

		void Undo(Editor::EditorContext& context) override { Apply(m_before); }
		void Redo(Editor::EditorContext& context) override { Apply(m_after); }
		std::string GetName() const override { return "Rename"; }

	private:
		void Apply(const std::string& name)
		{
			const GameObjectPtr gameObject = Editor::UndoTargets::FindObject(m_guid);
			if (gameObject != nullptr)
			{
				gameObject->SetName(name);
			}
		}

	private:
		ObjectGUID m_guid;
		std::string m_before;
		std::string m_after;
	};

	// A pasted camera is a copy, so it never takes over as the main camera
	void ClearMainCameraFlag(rapidjson::Value& entry)
	{
		const auto components = entry.FindMember("components");
		if (components == entry.MemberEnd() || !components->value.IsObject())
		{
			return;
		}

		const auto camera = components->value.FindMember(CameraComponent::GetComponentName());
		if (camera != components->value.MemberEnd() && camera->value.IsObject() && camera->value.HasMember("MainCamera"))
		{
			camera->value["MainCamera"].SetBool(false);
		}
	}
}

namespace Editor
{
	CreateGameObjectCommand::CreateGameObjectCommand(std::string name)
		: m_name(std::move(name))
	{
	}

	bool CreateGameObjectCommand::Execute(EditorContext& context)
	{
		RZE().GetActiveScene().AddGameObject(m_name);
		return true;
	}

	RenameGameObjectCommand::RenameGameObjectCommand(GameObjectPtr gameObject, std::string name)
		: m_gameObject(gameObject)
		, m_name(std::move(name))
	{
	}

	bool RenameGameObjectCommand::Execute(EditorContext& context)
	{
		if (m_gameObject == nullptr || m_gameObject->GetName() == m_name)
		{
			return false;
		}

		SetUndoRecord(std::make_unique<RenameRecord>(m_gameObject->GetGUID(), m_gameObject->GetName(), m_name));
		m_gameObject->SetName(m_name);
		return true;
	}

	DeleteGameObjectCommand::DeleteGameObjectCommand(GameObjectPtr gameObject)
		: m_gameObject(gameObject)
	{
	}

	bool DeleteGameObjectCommand::Execute(EditorContext& context)
	{
		if (m_gameObject == nullptr)
		{
			return false;
		}

		// While the object is still alive, so SelectionBoundsDisplay can turn off its bounds
		context.GetSelection().Clear();

		// Deferred by the scene to its next update
		RZE().GetActiveScene().RemoveGameObject(m_gameObject);
		return true;
	}

	AddComponentCommand::AddComponentCommand(GameObjectPtr gameObject, GameObjectComponentID componentID)
		: m_gameObject(gameObject)
		, m_componentID(componentID)
	{
	}

	bool AddComponentCommand::Execute(EditorContext& context)
	{
		// Null if the object already has one
		return m_gameObject != nullptr && m_gameObject->AddComponentByID(m_componentID) != nullptr;
	}

	AttachToParentCommand::AttachToParentCommand(GameObjectPtr child, GameObjectPtr parent)
		: m_child(child)
		, m_parent(parent)
	{
	}

	bool AttachToParentCommand::Execute(EditorContext& context)
	{
		if (m_child == nullptr || m_parent == nullptr || m_child->IsSelfOrAncestorOf(m_parent))
		{
			return false;
		}

		if (!m_child->IsRoot())
		{
			m_child->DetachFromParent();
		}

		m_child->AttachTo(m_parent);
		return true;
	}

	PasteGameObjectsCommand::PasteGameObjectsCommand(std::string sceneData, const Vector3D& worldOffset)
		: m_sceneData(std::move(sceneData))
		, m_worldOffset(worldOffset)
	{
	}

	bool PasteGameObjectsCommand::Execute(EditorContext& context)
	{
		rapidjson::Document document;
		document.Parse(m_sceneData.c_str());
		std::string error;
		if (document.HasParseError() || !SceneFormat::Upgrade(document, error))
		{
			return false;
		}

		// Parents are linked by the GUIDs in the copied data, so the loader keeps copied children under their
		// pasted parents, and a copied object keeps its original parent if that's still in the scene
		GameObjectLoader loader(RZE().GetActiveScene());
		rapidjson::Document::AllocatorType& allocator = document.GetAllocator();
		for (rapidjson::Value& entry : SceneFormat::GetObjects(document).GetArray())
		{
			const std::string pastedName = MakeUniqueName(entry["name"].GetString());
			entry["name"].SetString(pastedName.c_str(), static_cast<rapidjson::SizeType>(pastedName.size()), allocator);
			ClearMainCameraFlag(entry);
			loader.Load(entry);
		}

		for (const GameObjectPtr& gameObject : loader.GetTopLevelObjects())
		{
			MoveInWorld(gameObject, m_worldOffset);
		}

		if (loader.GetTopLevelObjects().empty())
		{
			return false;
		}

		context.GetSelection().SetSelection(loader.GetTopLevelObjects());
		return true;
	}

	DetachFromParentCommand::DetachFromParentCommand(GameObjectPtr gameObject)
		: m_gameObject(gameObject)
	{
	}

	bool DetachFromParentCommand::Execute(EditorContext& context)
	{
		if (m_gameObject == nullptr || m_gameObject->IsRoot())
		{
			return false;
		}

		m_gameObject->DetachFromParent();
		return true;
	}
}
