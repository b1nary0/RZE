#pragma once

#include <Commands/EditorCommand.h>

#include <Game/World/GameObject/GameObjectDefinitions.h>

#include <Utils/Math/Vector3D.h>

namespace Editor
{
	// Sets a GameObject's local position, rotation (Euler degrees) and scale. Does nothing if the object has been
	// destroyed. A gizmo drag pushes one of these each frame, all sharing the drag's interaction.
	class SetTransformCommand : public EditorCommand
	{
	public:
		SetTransformCommand(GameObjectPtr gameObject, const Vector3D& position, const Vector3D& rotation, const Vector3D& scale, InteractionID interaction);

		void Execute(EditorContext& context) override;
		const char* GetName() const override { return "Set Transform"; }

	private:
		GameObjectPtr m_gameObject;
		Vector3D m_position;
		Vector3D m_rotation;
		Vector3D m_scale;
	};
}
