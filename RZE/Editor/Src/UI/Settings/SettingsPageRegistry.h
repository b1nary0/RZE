#pragma once

#include <UI/Settings/SettingsPage.h>

#include <memory>
#include <vector>

// Registers a SettingsPage subclass so the Settings window shows it. Put it in the page's .cpp, at namespace scope.
// The page needs a default constructor.
// Like REGISTER_EDITOR_PANEL, registration runs from a static initializer that nothing else references, so it relies on
// Editor linking EditorLib with /WHOLEARCHIVE (see Editor.sharpmake.cs). Anything else linking EditorLib (e.g.
// UnitTests) gets no pages from it unless it does the same.
#define REGISTER_SETTINGS_PAGE(PageType) \
	static const ::Editor::SettingsPageRegistrar<PageType> s_settingsPageRegistrar_##PageType

namespace Editor
{
	namespace SettingsPageRegistry
	{
		using CreateFunction = std::unique_ptr<SettingsPage> (*)();

		void Register(CreateFunction create);

		// One of each registered page, sorted by name (registration order depends on link order)
		std::vector<std::unique_ptr<SettingsPage>> CreateAll();
	}

	template <typename TPage>
	class SettingsPageRegistrar
	{
	public:
		SettingsPageRegistrar()
		{
			SettingsPageRegistry::Register([]() -> std::unique_ptr<SettingsPage> { return std::make_unique<TPage>(); });
		}
	};
}
