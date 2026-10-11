#pragma once

#include <UI/Framework/EditorPanel.h>

#include <memory>
#include <vector>

// Registers an EditorPanel subclass so PanelManager creates it. Put it in the panel's .cpp, at namespace scope.
// The panel needs a static PanelDesc Describe() and a default constructor.
// Registration runs from a static initializer, so it only happens for code linked straight into the editor
// executable. If the panels ever move into a static library, the linker will drop them unless they're
// force-linked (e.g. /WHOLEARCHIVE).
#define REGISTER_EDITOR_PANEL(PanelType) \
	static const ::Editor::PanelRegistrar<PanelType> s_panelRegistrar_##PanelType

namespace Editor
{
	struct PanelRegistration
	{
		PanelDesc (*Describe)();
		std::unique_ptr<EditorPanel> (*Create)();
	};

	namespace PanelRegistry
	{
		void Register(const PanelRegistration& registration);
		// In registration order, which depends on link order; PanelManager sorts by name
		const std::vector<PanelRegistration>& GetAll();
	}

	template <typename TPanel>
	class PanelRegistrar
	{
	public:
		PanelRegistrar()
		{
			PanelRegistration registration;
			registration.Describe = &TPanel::Describe;
			registration.Create = []() -> std::unique_ptr<EditorPanel> { return std::make_unique<TPanel>(); };
			PanelRegistry::Register(registration);
		}
	};
}
