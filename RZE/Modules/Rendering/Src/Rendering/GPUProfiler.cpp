#include <Rendering/GPUProfiler.h>

#include <Utils/DebugUtils/Debug.h>

#include <algorithm>
#include <limits>

namespace Rendering
{
	namespace
	{
		// On the open-scope stack for a scope that didn't fit, so its EndScope still pops it
		constexpr U32 k_unprofiledScope = std::numeric_limits<U32>::max();
	}

	struct GPUProfiler::Frame
	{
		struct Scope
		{
			U32 ID = 0;
			U32 BeginIndex = 0;
			U32 EndIndex = 0;
			// The statistics segments inside it: from FirstSegment up to, not including, EndSegment
			U32 FirstSegment = 0;
			U32 EndSegment = 0;
		};

		std::unique_ptr<IGPUTimestamps> Timestamps;
		// None when frames aren't counted
		std::unique_ptr<IGPUStatistics> Statistics;
		std::vector<Scope> Scopes;
		// Indices into Scopes of the scopes begun and not yet ended, innermost last
		std::vector<U32> OpenScopes;
		// Timestamps written
		U32 Count = 0;
		// Statistics segments begun, and whether the last is still open
		U32 SegmentCount = 0;
		bool IsSegmentOpen = false;
		// Ended and not read back yet
		bool IsPending = false;
	};

	GPUProfiler::GPUProfiler() = default;
	GPUProfiler::~GPUProfiler() = default;

	void GPUProfiler::Initialize(const GPUTimestampsFactory& createTimestamps, const GPUStatisticsFactory& createStatistics)
	{
		for (std::unique_ptr<Frame>& frame : m_frames)
		{
			frame = std::make_unique<Frame>();
			frame->Timestamps = createTimestamps(k_capacity);
			if (createStatistics)
			{
				frame->Statistics = createStatistics(k_capacity);
			}
		}

		// Every frame counts or none does, so the averages never mix counted and uncounted frames
		m_hasStatistics = std::all_of(m_frames.begin(), m_frames.end(),
			[](const std::unique_ptr<Frame>& frame) { return frame->Statistics != nullptr; });
		if (!m_hasStatistics)
		{
			for (std::unique_ptr<Frame>& frame : m_frames)
			{
				frame->Statistics.reset();
			}
		}
	}

	void GPUProfiler::Release()
	{
		for (std::unique_ptr<Frame>& frame : m_frames)
		{
			frame.reset();
		}
		m_isFrameOpen = false;
		m_hasStatistics = false;
	}

	void GPUProfiler::BeginFrameIfNeeded()
	{
		if (m_isFrameOpen || m_frames[m_current] == nullptr)
		{
			return;
		}

		// Still waiting for the GPU to finish what it profiled k_frameCount frames ago: this frame is skipped
		Frame& frame = *m_frames[m_current];
		if (frame.IsPending)
		{
			return;
		}

		frame.Scopes.clear();
		frame.OpenScopes.clear();
		frame.Timestamps->Begin();
		frame.Timestamps->Write(0);
		frame.Count = 1;
		if (frame.Statistics != nullptr)
		{
			frame.Statistics->Begin();
		}
		frame.SegmentCount = 0;
		frame.IsSegmentOpen = false;
		m_isFrameOpen = true;
	}

	void GPUProfiler::BeginScope(U32 id)
	{
		if (!m_isFrameOpen)
		{
			return;
		}

		Frame& frame = *m_frames[m_current];
		// Room for this one's two, the ends of the scopes already open, and the frame's last. A scope inside one that
		// didn't fit needs more, so it doesn't fit either: only the innermost open scopes are ever unprofiled.
		const size_t needed = frame.Count + 2 + frame.OpenScopes.size() + 1;
		if (needed > k_capacity)
		{
			frame.OpenScopes.push_back(k_unprofiledScope);
			return;
		}

		frame.Timestamps->Write(frame.Count);
		frame.Scopes.push_back({ id, frame.Count, 0, frame.SegmentCount, 0 });
		frame.OpenScopes.push_back(static_cast<U32>(frame.Scopes.size() - 1));
		++frame.Count;
		// Its first segment is the one begun here
		SplitSegment(frame, true);
	}

	void GPUProfiler::EndScope()
	{
		if (!m_isFrameOpen)
		{
			return;
		}

		Frame& frame = *m_frames[m_current];
		AssertMsg(!frame.OpenScopes.empty(), "EndGPUScope without a BeginGPUScope");
		if (frame.OpenScopes.empty())
		{
			return;
		}

		const U32 scopeIndex = frame.OpenScopes.back();
		frame.OpenScopes.pop_back();
		if (scopeIndex == k_unprofiledScope)
		{
			return;
		}

		frame.Timestamps->Write(frame.Count);
		frame.Scopes[scopeIndex].EndIndex = frame.Count;
		++frame.Count;
		// Its last segment is the one ending here. The scope around it, if any, goes on counting in a new one.
		frame.Scopes[scopeIndex].EndSegment = frame.SegmentCount;
		SplitSegment(frame, !frame.OpenScopes.empty());
	}

	void GPUProfiler::SplitSegment(Frame& frame, bool beginNext)
	{
		if (frame.Statistics == nullptr)
		{
			return;
		}

		if (frame.IsSegmentOpen)
		{
			frame.Statistics->EndSegment();
			frame.IsSegmentOpen = false;
		}
		if (beginNext)
		{
			// At most two segments a profiled scope, which k_capacity's timestamps already limit to fewer than k_capacity
			AssertExpr(frame.SegmentCount < k_capacity);
			frame.Statistics->BeginSegment(frame.SegmentCount);
			++frame.SegmentCount;
			frame.IsSegmentOpen = true;
		}
	}

	bool GPUProfiler::EndFrame(GPUFrameProfile& outProfile)
	{
		if (m_isFrameOpen)
		{
			Frame& frame = *m_frames[m_current];
			AssertMsg(frame.OpenScopes.empty(), "A GPU scope is still open as the frame presents");

			frame.Timestamps->Write(frame.Count);
			++frame.Count;
			frame.Timestamps->End();
			if (frame.Statistics != nullptr)
			{
				// Only open here if a scope was left open, which the assert above reports
				SplitSegment(frame, false);
				frame.Statistics->End();
			}
			frame.IsPending = true;

			m_current = (m_current + 1) % k_frameCount;
			m_isFrameOpen = false;
		}

		bool hasProfile = false;
		ReadFinishedFrames(outProfile, hasProfile);
		return hasProfile;
	}

	void GPUProfiler::ReadFinishedFrames(GPUFrameProfile& outProfile, bool& outHasProfile)
	{
		// Oldest first: the GPU finishes frames in order, so the first not ready means none after it are
		while (m_frames[m_oldest] != nullptr && m_frames[m_oldest]->IsPending)
		{
			Frame& frame = *m_frames[m_oldest];
			// The timestamps first: on GL one check of their last says whether the frame can be done
			const EGPUQueryReadResult timesResult = frame.Timestamps->Read(frame.Count, m_seconds);
			if (timesResult == EGPUQueryReadResult::NotReady)
			{
				return;
			}
			EGPUQueryReadResult statisticsResult = EGPUQueryReadResult::Ready;
			if (frame.Statistics != nullptr)
			{
				statisticsResult = frame.Statistics->Read(frame.SegmentCount, m_segments);
				if (statisticsResult == EGPUQueryReadResult::NotReady)
				{
					return;
				}
			}

			frame.IsPending = false;
			m_oldest = (m_oldest + 1) % k_frameCount;
			// Times and counts describe the same frames, so a frame whose times or counts can't be trusted is dropped whole
			if (timesResult == EGPUQueryReadResult::Invalid || statisticsResult == EGPUQueryReadResult::Invalid)
			{
				continue;
			}

			outProfile.FrameMS = m_seconds[frame.Count - 1] * 1000.0;
			outProfile.HasStatistics = frame.Statistics != nullptr;
			std::fill(outProfile.ScopeMS.begin(), outProfile.ScopeMS.end(), 0.0);
			std::fill(outProfile.ScopeStatistics.begin(), outProfile.ScopeStatistics.end(), GPUPipelineStatistics());
			for (const Frame::Scope& scope : frame.Scopes)
			{
				if (scope.ID >= outProfile.ScopeMS.size())
				{
					outProfile.ScopeMS.resize(scope.ID + 1, 0.0);
					outProfile.ScopeStatistics.resize(scope.ID + 1);
				}
				outProfile.ScopeMS[scope.ID] += (m_seconds[scope.EndIndex] - m_seconds[scope.BeginIndex]) * 1000.0;
				if (outProfile.HasStatistics)
				{
					for (U32 segment = scope.FirstSegment; segment < scope.EndSegment; ++segment)
					{
						outProfile.ScopeStatistics[scope.ID] += m_segments[segment];
					}
				}
			}

			// Segments never overlap, so their sum counts each draw once however the scopes nest
			outProfile.FrameStatistics = GPUPipelineStatistics();
			if (outProfile.HasStatistics)
			{
				for (U32 segment = 0; segment < frame.SegmentCount; ++segment)
				{
					outProfile.FrameStatistics += m_segments[segment];
				}
			}
			outHasProfile = true;
		}
	}
}
