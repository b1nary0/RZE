#pragma once

#include <Game/World/GameObject/GameObjectDefinitions.h>

#include <functional>

// Registers a custom inspector section for a component type: what the inspector draws after the component's
// described properties, for what descriptors can't express (previews, read-only trees, buttons). Put it in a .cpp
// at namespace scope:
//
//   void DrawCameraPreview(Editor::EditorContext& context, CameraComponent& camera) { ... }
//   REGISTER_COMPONENT_INSPECTOR(CameraComponent, DrawCameraPreview);
//
// Like REGISTER_EDITOR_PANEL, this relies on a static initializer, so it only works in code linked straight into
// the editor executable.
#define REGISTER_COMPONENT_INSPECTOR(ComponentType, InspectorFunction) \
	static const ::Editor::ComponentInspectorRegistrar<ComponentType> s_componentInspector_##ComponentType(InspectorFunction)

namespace Editor
{
	class EditorContext;

	namespace ComponentInspectors
	{
		using Inspector = std::function<void(EditorContext&, GameObjectComponentBase&)>;
		using GetComponentID = GameObjectComponentID (*)();

		// getComponentID is called lazily, the first time an inspector is looked up, rather than during static
		// initialization
		void Register(GetComponentID getComponentID, Inspector inspector);

		// Null if the component type has no custom inspector
		const Inspector* Find(GameObjectComponentID componentID);
	}

	template <typename TComponent>
	class ComponentInspectorRegistrar
	{
	public:
		explicit ComponentInspectorRegistrar(void (*inspector)(EditorContext&, TComponent&))
		{
			ComponentInspectors::Register(&TComponent::GetID, [inspector](EditorContext& context, GameObjectComponentBase& component)
				{
					inspector(context, static_cast<TComponent&>(component));
				});
		}
	};
}
