#include <Commands/SceneCommands.h>

#include <Commands/ParkedSubtree.h>

#include <UI/Framework/EditorContext.h>

#include <RZE.h>

#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/TransformComponent.h>

#include <vector>

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

	// Objects a command added or removed: undo parks or restores the very same objects (see ParkedSubtree)
	class ParkRecord : public Editor::UndoRecord
	{
	public:
		// isParkedWhenDone: true for a removal (the objects are parked after the command and after a redo), false for
		// an addition (parked after an undo)
		ParkRecord(std::string name, std::vector<Editor::ParkedSubtree> subtrees, bool isParkedWhenDone)
			: m_name(std::move(name))
			, m_subtrees(std::move(subtrees))
			, m_isParkedWhenDone(isParkedWhenDone)
		{
		}

		void Undo(Editor::EditorContext& context) override
		{
			if (m_isParkedWhenDone)
			{
				RestoreAll();
			}
			else
			{
				ParkAll(context);
			}
		}

		void Redo(Editor::EditorContext& context) override
		{
			if (m_isParkedWhenDone)
			{
				ParkAll(context);
			}
			else
			{
				RestoreAll();
			}
		}

		std::string GetName() const override { return m_name; }

		void OnDropped() override
		{
			for (Editor::ParkedSubtree& subtree : m_subtrees)
			{
				subtree.DestroyIfParked(RZE().GetActiveScene());
			}
		}

	private:
		// Parked last first and restored first first, so objects sharing a parent go back at the indices they had
		void ParkAll(Editor::EditorContext& context)
		{
			for (auto it = m_subtrees.rbegin(); it != m_subtrees.rend(); ++it)
			{
				it->Park(RZE().GetActiveScene(), context.GetSelection());
			}
		}

		void RestoreAll()
		{
			for (Editor::ParkedSubtree& subtree : m_subtrees)
			{
				subtree.Restore(RZE().GetActiveScene());
			}
		}

	private:
		std::string m_name;
		std::vector<Editor::ParkedSubtree> m_subtrees;
		bool m_isParkedWhenDone;
	};

	class AddComponentRecord : public Editor::UndoRecord
	{
	public:
		AddComponentRecord(ObjectGUID guid, GameObjectComponentID componentID)
			: m_guid(guid)
			, m_componentID(componentID)
		{
		}

		// The component's state goes with it. Edits to it made afterwards are later steps, which redo applies again
		// once this has added a new one.
		void Undo(Editor::EditorContext& context) override
		{
			const GameObjectPtr gameObject = Editor::UndoTargets::FindObject(m_guid);
			if (gameObject != nullptr)
			{
				gameObject->RemoveComponentByID(m_componentID);
			}
		}

		void Redo(Editor::EditorContext& context) override
		{
			const GameObjectPtr gameObject = Editor::UndoTargets::FindObject(m_guid);
			if (gameObject != nullptr)
			{
				gameObject->AddComponentByID(m_componentID);
			}
		}

		std::string GetName() const override { return "Add Component"; }

	private:
		ObjectGUID m_guid;
		GameObjectComponentID m_componentID;
	};

	// Where an object sits in the hierarchy: its parent (invalid for a root) and its index among the parent's children
	struct HierarchyPlace
	{
		ObjectGUID ParentGUID = ObjectGUIDs::k_invalid;
		int ChildIndex = -1;

		static HierarchyPlace Of(const GameObjectPtr& gameObject)
		{
			HierarchyPlace place;
			const GameObjectPtr parent = gameObject->GetParent();
			if (parent != nullptr)
			{
				place.ParentGUID = parent->GetGUID();
				place.ChildIndex = parent->GetChildIndex(*gameObject);
			}
			return place;
		}
	};

	class ChangeParentRecord : public Editor::UndoRecord
	{
	public:
		ChangeParentRecord(std::string name, ObjectGUID guid, const HierarchyPlace& before, const HierarchyPlace& after)
			: m_name(std::move(name))
			, m_guid(guid)
			, m_before(before)
			, m_after(after)
		{
		}

		void Undo(Editor::EditorContext& context) override { Apply(m_before); }
		void Redo(Editor::EditorContext& context) override { Apply(m_after); }
		std::string GetName() const override { return m_name; }

	private:
		void Apply(const HierarchyPlace& place)
		{
			const GameObjectPtr gameObject = Editor::UndoTargets::FindObject(m_guid);
			if (gameObject == nullptr)
			{
				return;
			}

			if (!gameObject->IsRoot())
			{
				gameObject->DetachFromParent();
			}

			if (place.ParentGUID != ObjectGUIDs::k_invalid)
			{
				const GameObjectPtr parent = Editor::UndoTargets::FindObject(place.ParentGUID);
				if (parent != nullptr)
				{
					gameObject->AttachTo(parent, place.ChildIndex);
				}
			}
		}

	private:
		std::string m_name;
		ObjectGUID m_guid;
		HierarchyPlace m_before;
		HierarchyPlace m_after;
	};

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
}

namespace Editor
{
	CreateGameObjectCommand::CreateGameObjectCommand(std::string name)
		: m_name(std::move(name))
	{
	}

	bool CreateGameObjectCommand::Execute(EditorContext& context)
	{
		const GameObjectPtr gameObject = RZE().GetActiveScene().AddGameObject(m_name);
		SetUndoRecord(std::make_unique<ParkRecord>("Create GameObject", std::vector<ParkedSubtree>{ ParkedSubtree(gameObject) }, false));
		return true;
	}

	RenameGameObjectCommand::RenameGameObjectCommand(GameObjectPtr gameObject, std::string name)
		: m_gameObject(gameObject)
		, m_name(std::move(name))
	{
	}

	bool RenameGameObjectCommand::Execute(EditorContext& context)
	{
		if (!UndoTargets::IsInScene(m_gameObject) || m_gameObject->GetName() == m_name)
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
		if (!UndoTargets::IsInScene(m_gameObject))
		{
			return false;
		}

		// Parked rather than destroyed, so undo puts back the same objects; destroyed once the step leaves the history
		ParkedSubtree subtree(m_gameObject);
		subtree.Park(RZE().GetActiveScene(), context.GetSelection());
		SetUndoRecord(std::make_unique<ParkRecord>("Delete GameObject", std::vector<ParkedSubtree>{ subtree }, true));
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
		if (!UndoTargets::IsInScene(m_gameObject) || m_gameObject->AddComponentByID(m_componentID) == nullptr)
		{
			return false;
		}

		SetUndoRecord(std::make_unique<AddComponentRecord>(m_gameObject->GetGUID(), m_componentID));
		return true;
	}

	AttachToParentCommand::AttachToParentCommand(GameObjectPtr child, GameObjectPtr parent)
		: m_child(child)
		, m_parent(parent)
	{
	}

	bool AttachToParentCommand::Execute(EditorContext& context)
	{
		if (!UndoTargets::IsInScene(m_child) || !UndoTargets::IsInScene(m_parent) || m_child->IsSelfOrAncestorOf(m_parent))
		{
			return false;
		}

		const HierarchyPlace before = HierarchyPlace::Of(m_child);
		if (!m_child->IsRoot())
		{
			m_child->DetachFromParent();
		}

		m_child->AttachTo(m_parent);
		SetUndoRecord(std::make_unique<ChangeParentRecord>("Attach To Parent", m_child->GetGUID(), before, HierarchyPlace::Of(m_child)));
		return true;
	}

	PasteGameObjectsCommand::PasteGameObjectsCommand(std::vector<GameObjectSnapshot> snapshots, const Vector3D& worldOffset)
		: m_snapshots(std::move(snapshots))
		, m_worldOffset(worldOffset)
	{
	}

	bool PasteGameObjectsCommand::Execute(EditorContext& context)
	{
		if (m_snapshots.empty())
		{
			return false;
		}

		GameScene& scene = RZE().GetActiveScene();
		std::vector<GameObjectPtr> pasted;
		for (const GameObjectSnapshot& snapshot : m_snapshots)
		{
			// Null if the original's parent is gone, or the copy came from another scene
			const GameObjectPtr parent = snapshot.GetParentGUID() != ObjectGUIDs::k_invalid
				? scene.FindGameObjectByGUID(snapshot.GetParentGUID()) : GameObjectPtr();
			const GameObjectPtr gameObject = snapshot.Create(scene, parent, &MakeUniqueName);
			MoveInWorld(gameObject, m_worldOffset);
			pasted.push_back(gameObject);
		}

		std::vector<ParkedSubtree> subtrees;
		for (const GameObjectPtr& gameObject : pasted)
		{
			subtrees.emplace_back(gameObject);
		}
		SetUndoRecord(std::make_unique<ParkRecord>("Paste", std::move(subtrees), false));

		context.GetSelection().SetSelection(pasted);
		return true;
	}

	DetachFromParentCommand::DetachFromParentCommand(GameObjectPtr gameObject)
		: m_gameObject(gameObject)
	{
	}

	bool DetachFromParentCommand::Execute(EditorContext& context)
	{
		if (!UndoTargets::IsInScene(m_gameObject) || m_gameObject->IsRoot())
		{
			return false;
		}

		const HierarchyPlace before = HierarchyPlace::Of(m_gameObject);
		m_gameObject->DetachFromParent();
		SetUndoRecord(std::make_unique<ChangeParentRecord>("Detach From Parent", m_gameObject->GetGUID(), before, HierarchyPlace()));
		return true;
	}
}
