#include <Services/ProjectWatcher.h>

#include <UI/Framework/EditorEvents.h>

#include <DebugUtils/DebugServices.h>

namespace Editor
{
	ProjectWatcher::ProjectWatcher(EditorEvents& events)
		: m_events(events)
	{
	}

	void ProjectWatcher::Start()
	{
		m_watchers.clear();
		for (const char* folder : { "Assets", "ProjectData" })
		{
			FolderWatcher watcher{ Filepath(folder) };
			if (!watcher.IsWatching())
			{
				DebugServices::Get().Trace(LogChannel::Info, std::string("Can't watch ") + folder + " for changes; use Refresh in the Content Browser to see them.");
				continue;
			}
			m_watchers.push_back(std::move(watcher));
		}
	}

	void ProjectWatcher::Update()
	{
		for (FolderWatcher& watcher : m_watchers)
		{
			// Each is polled, so none stays signalled
			if (watcher.PollChanged())
			{
				m_hasUntoldChange = true;
			}
		}

		const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
		if (m_hasUntoldChange && now - m_lastTold >= k_minInterval)
		{
			m_hasUntoldChange = false;
			m_lastTold = now;
			m_events.Publish(ProjectFilesChangedEvent{});
		}
	}
}
