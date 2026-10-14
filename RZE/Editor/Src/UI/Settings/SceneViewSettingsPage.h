#pragma once

#include <UI/Settings/SettingsPage.h>

namespace Editor
{
	// The transform gizmo's snap steps, which the scene view's toolbar also shows (EditorSettings)
	class SceneViewSettingsPage : public SettingsPage
	{
	public:
		std::string GetName() const override { return "Scene View"; }
		void Draw(EditorContext& context) override;
	};
}
