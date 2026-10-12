#pragma once

#include <UI/Framework/EditorPanel.h>

#include <Utils/Platform/Timers/HiResTimer.h>

#include <string>
#include <vector>

namespace Editor
{
	// Every loaded resource and its reference count
	class ResourceMonitorPanel : public EditorPanel
	{
	public:
		static PanelDesc Describe();

	private:
		struct ResourceRow
		{
			std::string m_name;
			int m_refCount = 0;
		};

	private:
		void Update(EditorContext& context) override;
		void Draw(EditorContext& context) override;
		void PushWindowStyle(PanelWindowStyle& style) const override;

	private:
		// Sorted by name. Rebuilt a few times a second rather than every frame, since copying and sorting the
		// names is costly in debug builds; reuses its allocation.
		std::vector<ResourceRow> m_rows;
		HiResTimer m_sinceRefresh;
		bool m_hasRows = false;
	};
}
