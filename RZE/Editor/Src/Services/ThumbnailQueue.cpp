#include <Services/ThumbnailQueue.h>

#include <Services/Thumbnails.h>

#include <Utils/StringUtils.h>

#include <algorithm>

namespace
{
	bool IsSameAsset(const Filepath& first, const Filepath& second)
	{
		return StringUtils::EqualsIgnoringCase(first.GetRelativePath(), second.GetRelativePath());
	}
}

namespace Editor
{
	ThumbnailQueue::ThumbnailQueue(ImageFor imageFor)
		: m_imageFor(imageFor ? std::move(imageFor) : ImageFor(&Thumbnails::FindFor))
	{
	}

	void ThumbnailQueue::Add(const Filepath& asset, bool force)
	{
		if (m_hasCurrent && IsSameAsset(m_current, asset))
		{
			m_isCurrentAskedForAgain = true;
			return;
		}

		const auto queued = std::find_if(m_queued.begin(), m_queued.end(),
			[&asset](const Entry& entry) { return IsSameAsset(entry.Asset, asset); });
		if (queued != m_queued.end())
		{
			queued->Force = queued->Force || force;
			return;
		}

		m_queued.push_back({ asset, force });
	}

	bool ThumbnailQueue::TakeNext(Filepath& outAsset)
	{
		while (!m_queued.empty())
		{
			const Entry entry = m_queued.front();
			m_queued.pop_front();

			if (!entry.Force && Thumbnails::IsUpToDate(entry.Asset, m_imageFor(entry.Asset)))
			{
				++m_skippedCount;
				continue;
			}

			m_current = entry.Asset;
			m_hasCurrent = true;
			m_isCurrentAskedForAgain = false;
			outAsset = entry.Asset;
			return true;
		}

		return false;
	}

	void ThumbnailQueue::FinishCurrent()
	{
		if (!m_hasCurrent)
		{
			return;
		}

		m_hasCurrent = false;
		if (m_isCurrentAskedForAgain)
		{
			m_isCurrentAskedForAgain = false;
			Add(m_current, true);
		}
	}

	size_t ThumbnailQueue::TakeSkippedCount()
	{
		const size_t skippedCount = m_skippedCount;
		m_skippedCount = 0;
		return skippedCount;
	}

	void ThumbnailQueue::Clear()
	{
		m_queued.clear();
		m_hasCurrent = false;
		m_isCurrentAskedForAgain = false;
	}
}
