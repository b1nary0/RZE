#include <Rendering/GPUTimer.h>

#include <Utils/DebugUtils/Debug.h>

#include <algorithm>
#include <limits>

namespace Rendering
{
	namespace
	{
		// On the open-scope stack for a scope that didn't fit, so its EndScope still pops it
		constexpr U32 k_untimedScope = std::numeric_limits<U32>::max();
	}

	struct GPUTimer::Frame
	{
		struct Scope
		{
			U32 ID = 0;
			U32 BeginIndex = 0;
			U32 EndIndex = 0;
		};

		std::unique_ptr<IGPUTimestamps> Timestamps;
		std::vector<Scope> Scopes;
		// Indices into Scopes of the scopes begun and not yet ended, innermost last
		std::vector<U32> OpenScopes;
		// Timestamps written
		U32 Count = 0;
		// Ended and not read back yet
		bool IsPending = false;
	};

	GPUTimer::GPUTimer() = default;
	GPUTimer::~GPUTimer() = default;

	void GPUTimer::Initialize(const GPUTimestampsFactory& createTimestamps)
	{
		for (std::unique_ptr<Frame>& frame : m_frames)
		{
			frame = std::make_unique<Frame>();
			frame->Timestamps = createTimestamps(k_capacity);
		}
	}

	void GPUTimer::Release()
	{
		for (std::unique_ptr<Frame>& frame : m_frames)
		{
			frame.reset();
		}
		m_isFrameOpen = false;
	}

	void GPUTimer::BeginFrameIfNeeded()
	{
		if (m_isFrameOpen || m_frames[m_current] == nullptr)
		{
			return;
		}

		// Still waiting for the GPU to finish what it timed k_frameCount frames ago: this frame goes untimed
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
		m_isFrameOpen = true;
	}

	void GPUTimer::BeginScope(U32 id)
	{
		if (!m_isFrameOpen)
		{
			return;
		}

		Frame& frame = *m_frames[m_current];
		// Room for this one's two, the ends of the scopes already open, and the frame's last
		const size_t needed = frame.Count + 2 + frame.OpenScopes.size() + 1;
		if (needed > k_capacity)
		{
			frame.OpenScopes.push_back(k_untimedScope);
			return;
		}

		frame.Timestamps->Write(frame.Count);
		frame.Scopes.push_back({ id, frame.Count, 0 });
		frame.OpenScopes.push_back(static_cast<U32>(frame.Scopes.size() - 1));
		++frame.Count;
	}

	void GPUTimer::EndScope()
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
		if (scopeIndex == k_untimedScope)
		{
			return;
		}

		frame.Timestamps->Write(frame.Count);
		frame.Scopes[scopeIndex].EndIndex = frame.Count;
		++frame.Count;
	}

	bool GPUTimer::EndFrame(GPUFrameTimes& outTimes)
	{
		if (m_isFrameOpen)
		{
			Frame& frame = *m_frames[m_current];
			AssertMsg(frame.OpenScopes.empty(), "A GPU scope is still open as the frame presents");

			frame.Timestamps->Write(frame.Count);
			++frame.Count;
			frame.Timestamps->End();
			frame.IsPending = true;

			m_current = (m_current + 1) % k_frameCount;
			m_isFrameOpen = false;
		}

		bool hasTimes = false;
		ReadFinishedFrames(outTimes, hasTimes);
		return hasTimes;
	}

	void GPUTimer::ReadFinishedFrames(GPUFrameTimes& outTimes, bool& outHasTimes)
	{
		// Oldest first: the GPU finishes frames in order, so the first not ready means none after it are
		while (m_frames[m_oldest] != nullptr && m_frames[m_oldest]->IsPending)
		{
			Frame& frame = *m_frames[m_oldest];
			const EGPUTimestampsReadResult result = frame.Timestamps->Read(frame.Count, m_seconds);
			if (result == EGPUTimestampsReadResult::NotReady)
			{
				return;
			}

			frame.IsPending = false;
			m_oldest = (m_oldest + 1) % k_frameCount;
			if (result == EGPUTimestampsReadResult::Invalid)
			{
				continue;
			}

			outTimes.FrameMS = m_seconds[frame.Count - 1] * 1000.0;
			std::fill(outTimes.ScopeMS.begin(), outTimes.ScopeMS.end(), 0.0);
			for (const Frame::Scope& scope : frame.Scopes)
			{
				if (scope.ID >= outTimes.ScopeMS.size())
				{
					outTimes.ScopeMS.resize(scope.ID + 1, 0.0);
				}
				outTimes.ScopeMS[scope.ID] += (m_seconds[scope.EndIndex] - m_seconds[scope.BeginIndex]) * 1000.0;
			}
			outHasTimes = true;
		}
	}
}
