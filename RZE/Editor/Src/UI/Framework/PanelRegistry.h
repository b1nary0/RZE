#pragma once

#include <UI/Framework/EditorPanel.h>

#include <memory>
#include <vector>

// Registers an EditorPanel subclass so PanelManager creates it. Put it in the panel's .cpp, at namespace scope.
// The panel needs a static PanelDesc Describe() and a default constructor.
// Registration runs from a static initializer that nothing else references. Panels are built into EditorLib, a
// static library, so the linker would drop them; Editor links EditorLib with /WHOLEARCHIVE to keep them (see
// Editor.sharpmake.cs). Anything else linking EditorLib (e.g. UnitTests) gets no panels unless it does the same.
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
