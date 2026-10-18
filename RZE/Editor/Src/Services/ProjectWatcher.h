#pragma once

#include <Utils/Platform/FolderWatcher.h>

#include <chrono>
#include <vector>

namespace Editor
{
	class EditorEvents;

	// Publishes ProjectFilesChangedEvent when anything in the project's Assets or ProjectData folders changes, from
	// the editor or from outside it. A burst of changes, like SourceAssetBurner writing meshes or a batch of
	// thumbnails, is told at most once per k_minInterval. Owned by EditorContext.
	class ProjectWatcher
	{
	public:
		static constexpr std::chrono::milliseconds k_minInterval{ 250 };

		explicit ProjectWatcher(EditorEvents& events);

	public:
		// Starts watching. Not from the constructor, so tests that make an EditorContext don't watch anything.
		void Start();

		// Once per frame, from EditorContext::Update
		void Update();

	private:
		EditorEvents& m_events;
		std::vector<FolderWatcher> m_watchers;
		// A change not told yet, since one was told less than k_minInterval ago
		bool m_hasUntoldChange = false;
		std::chrono::steady_clock::time_point m_lastTold;
	};
}
