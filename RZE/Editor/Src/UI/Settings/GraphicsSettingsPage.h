#pragma once

#include <UI/Settings/SettingsPage.h>

namespace Editor
{
	// How the main view renders (RenderView): anti-aliasing, ambient occlusion, global illumination, what it shows and
	// auto exposure, changed while the editor runs. Engine.ini's [Rendering] sets them at start-up.
	class GraphicsSettingsPage : public SettingsPage
	{
	public:
		std::string GetName() const override { return "Graphics"; }
		void Draw(EditorContext& context) override;
	};
}
