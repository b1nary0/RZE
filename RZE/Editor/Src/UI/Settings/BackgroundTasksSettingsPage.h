#pragma once

#include <UI/Settings/SettingsPage.h>

namespace Editor
{
	// Which tasks the status bar shows (the async operation manager's AsyncOperationFilter), changed while the editor
	// runs. Engine.ini's [BackgroundTasks] sets them at start-up.
	class BackgroundTasksSettingsPage : public SettingsPage
	{
	public:
		std::string GetName() const override { return "Background Tasks"; }
		void Draw(EditorContext& context) override;
	};
}
