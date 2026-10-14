#pragma once

#include <string>

namespace Editor
{
	class EditorContext;

	// One page of the Settings window (SettingsPanel), listed by its name. Register a page by putting
	// REGISTER_SETTINGS_PAGE(PageType) in its .cpp; the Settings window creates one of each.
	class SettingsPage
	{
	public:
		SettingsPage() = default;
		virtual ~SettingsPage() = default;

		SettingsPage(const SettingsPage&) = delete;
		SettingsPage& operator=(const SettingsPage&) = delete;

	public:
		// In the page list, and as the page's heading
		virtual std::string GetName() const = 0;

		// The page's contents, while it's the one shown
		virtual void Draw(EditorContext& context) = 0;
	};
}
