#pragma once

#include <UI/Framework/EditorPanel.h>

namespace Editor
{
	// Every loaded resource and its reference count
	class ResourceMonitorPanel : public EditorPanel
	{
	public:
		static PanelDesc Describe();

	private:
		void Draw(EditorContext& context) override;
		void PushWindowStyle(PanelWindowStyle& style) const override;
	};
}
