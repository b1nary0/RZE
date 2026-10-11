#pragma once

#include <UI/Framework/EditorSelection.h>

namespace Editor
{
	// Editor-wide state and services that panels share, so they don't reach into EditorApp or each other.
	// Owned by EditorApp and handed to each panel when it's displayed.
	class EditorContext
	{
	public:
		EditorContext() = default;
		~EditorContext() = default;

		EditorContext(const EditorContext&) = delete;
		EditorContext& operator=(const EditorContext&) = delete;

	public:
		EditorSelection& GetSelection() { return m_selection; }
		const EditorSelection& GetSelection() const { return m_selection; }

	private:
		EditorSelection m_selection;
	};
}
