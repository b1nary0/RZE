#pragma once

#include <Commands/EditorCommand.h>

#include <Game/World/GameObject/GameObjectDefinitions.h>

#include <string>

// Structural changes to the scene's GameObjects. Each does nothing if the object it targets has been destroyed.
namespace Editor
{
	class CreateGameObjectCommand : public EditorCommand
	{
	public:
		explicit CreateGameObjectCommand(std::string name);

		void Execute(EditorContext& context) override;
		const char* GetName() const override { return "Create GameObject"; }

	private:
		std::string m_name;
	};

	class RenameGameObjectCommand : public EditorCommand
	{
	public:
		RenameGameObjectCommand(GameObjectPtr gameObject, std::string name);

		void Execute(EditorContext& context) override;
		const char* GetName() const override { return "Rename GameObject"; }

	private:
		GameObjectPtr m_gameObject;
		std::string m_name;
	};

	// Clears the selection, since the deleted object (or its children) may be in it
	class DeleteGameObjectCommand : public EditorCommand
	{
	public:
		explicit DeleteGameObjectCommand(GameObjectPtr gameObject);

		void Execute(EditorContext& context) override;
		const char* GetName() const override { return "Delete GameObject"; }

	private:
		GameObjectPtr m_gameObject;
	};

	class AddComponentCommand : public EditorCommand
	{
	public:
		AddComponentCommand(GameObjectPtr gameObject, GameObjectComponentID componentID);

		void Execute(EditorContext& context) override;
		const char* GetName() const override { return "Add Component"; }

	private:
		GameObjectPtr m_gameObject;
		GameObjectComponentID m_componentID;
	};

	// Detaches the child from any current parent first. Does nothing if parent is the child itself or one of its
	// descendants, which would make a cycle.
	class AttachToParentCommand : public EditorCommand
	{
	public:
		AttachToParentCommand(GameObjectPtr child, GameObjectPtr parent);

		void Execute(EditorContext& context) override;
		const char* GetName() const override { return "Attach To Parent"; }

	private:
		GameObjectPtr m_child;
		GameObjectPtr m_parent;
	};

	// Adds the GameObjects in sceneData (the scene format, from GameObjectClipboard) as new objects and selects the
	// top-level ones. Each gets a unique name ("Tank (1)") and a new GUID if its copied one is taken; copied children
	// stay under their pasted parents, and a top-level object keeps its parent if that's still in the scene.
	class PasteGameObjectsCommand : public EditorCommand
	{
	public:
		explicit PasteGameObjectsCommand(std::string sceneData);

		void Execute(EditorContext& context) override;
		const char* GetName() const override { return "Paste GameObjects"; }

	private:
		std::string m_sceneData;
	};

	class DetachFromParentCommand : public EditorCommand
	{
	public:
		explicit DetachFromParentCommand(GameObjectPtr gameObject);

		void Execute(EditorContext& context) override;
		const char* GetName() const override { return "Detach From Parent"; }

	private:
		GameObjectPtr m_gameObject;
	};
}
