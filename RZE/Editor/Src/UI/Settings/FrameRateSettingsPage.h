#pragma once

#include <UI/Settings/SettingsPage.h>

namespace Editor
{
	// Vsync, the frame rate caps and the UI's rate (FrameSettings), changed while the editor runs. Engine.ini sets them at
	// start-up; the status bar shows what they come to.
	class FrameRateSettingsPage : public SettingsPage
	{
	public:
		std::string GetName() const override { return "Frame Rate"; }
		void Draw(EditorContext& context) override;
	};
}
