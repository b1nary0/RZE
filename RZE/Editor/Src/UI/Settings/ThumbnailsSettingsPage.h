#pragma once

#include <UI/Settings/SettingsPage.h>

#include <Services/Thumbnails.h>

#include <Utils/Platform/Filepath.h>
#include <Utils/Platform/Timers/HiResTimer.h>

#include <vector>

namespace Editor
{
	// How many scene and mesh thumbnails are up to date, with buttons that render the rest, or all of them again, in
	// the background (ThumbnailRenderer)
	class ThumbnailsSettingsPage : public SettingsPage
	{
	public:
		ThumbnailsSettingsPage();

	public:
		std::string GetName() const override { return "Thumbnails"; }
		void Draw(EditorContext& context) override;

	private:
		struct Kind
		{
			const char* Heading;
			Filepath AssetFolder;
			const char* Extension;
			const ThumbnailFolder* ImageFolder;

			// As of the last refresh
			std::vector<Filepath> Assets;
			Thumbnails::Coverage Counts;
		};

	private:
		void RefreshCounts();
		void DrawKind(EditorContext& context, Kind& kind);

	private:
		std::vector<Kind> m_kinds;
		// Thumbnails are written in the background, so the counts are refreshed now and then while the page shows
		HiResTimer m_sinceRefresh;
		bool m_hasCounts = false;
	};
}
