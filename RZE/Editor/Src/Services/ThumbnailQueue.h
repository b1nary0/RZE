#pragma once

#include <Utils/Platform/Filepath.h>

#include <deque>
#include <functional>

namespace Editor
{
	// The assets whose thumbnails are to be rendered, in the order they were asked for, each once. Holds no rendering,
	// so it can be tested.
	class ThumbnailQueue
	{
	public:
		// The thumbnail image of an asset
		using ImageFor = std::function<Filepath(const Filepath& asset)>;

	public:
		// By default, Thumbnails::FindFor
		explicit ThumbnailQueue(ImageFor imageFor = nullptr);

	public:
		// Queues asset. Asking for one that's already queued only carries force over. Asking for the one being rendered
		// puts it back on the queue, forced, once it's done (FinishCurrent), since it may have changed meanwhile.
		void Add(const Filepath& asset, bool force);

		// The next asset whose thumbnail needs rendering, which becomes the current one. Those whose image is up to date
		// (Thumbnails::IsUpToDate) are skipped, unless forced. False when there are none left.
		bool TakeNext(Filepath& outAsset);
		// The current one is done; it goes back on the queue if it was asked for meanwhile
		void FinishCurrent();

		// Not counting the current one, and including any that will turn out to be up to date
		size_t GetQueuedCount() const { return m_queued.size(); }
		bool HasCurrent() const { return m_hasCurrent; }
		// How many TakeNext has skipped since this was last called
		size_t TakeSkippedCount();

		// Forgets everything queued, and the current one
		void Clear();

	private:
		struct Entry
		{
			Filepath Asset;
			bool Force = false;
		};

		ImageFor m_imageFor;
		std::deque<Entry> m_queued;

		Filepath m_current;
		bool m_hasCurrent = false;
		bool m_isCurrentAskedForAgain = false;

		size_t m_skippedCount = 0;
	};
}
