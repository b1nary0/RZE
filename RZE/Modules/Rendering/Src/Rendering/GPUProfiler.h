#pragma once

#include <Utils/DebugUtils/DebugConfig.h>

// Render stats only
#ifdef RZE_RENDER_STATS

#include <Rendering/Driver/GPUStatistics.h>
#include <Rendering/Driver/GPUTimestamps.h>

#include <Utils/PrimitiveDefs.h>

#include <array>
#include <functional>
#include <memory>
#include <vector>

namespace Rendering
{
	// Makes one frame's timestamps, with room for capacity of them: IGFXDevice::CreateGPUTimestamps, or a test's own
	using GPUTimestampsFactory = std::function<std::unique_ptr<IGPUTimestamps>(U32 capacity)>;
	// Makes one frame's pipeline statistics, with room for capacity segments: IGFXDevice::CreateGPUStatistics, or a
	// test's own. Empty, or returning none, when there are none to count.
	using GPUStatisticsFactory = std::function<std::unique_ptr<IGPUStatistics>(U32 capacity)>;

	// One frame on the GPU (GPUProfiler): its times and, if it counted them, its statistics
	struct GPUFrameProfile
	{
		// From the frame's first command to its last before presenting
		double FrameMS = 0.0;
		// Indexed by scope ID (Renderer::BeginGPUScope), each scope with that ID summed over the frame; 0 for an ID the
		// frame didn't time
		std::vector<double> ScopeMS;

		// Whether the statistics below were counted: false while they're off, and where the device has none
		bool HasStatistics = false;
		// Indexed and summed like ScopeMS
		std::vector<GPUPipelineStatistics> ScopeStatistics;
		// Every scope's, counted once where they nest: the frame's draws, but for any outside every scope
		GPUPipelineStatistics FrameStatistics;
	};

	// Times frames, and scopes within them, on the GPU, through the device's timestamps, and counts what the GPU did in
	// each scope through its pipeline statistics, the same way on every graphics API. A frame is read back a few frames
	// later, once the GPU has finished it, so profiling never makes the render thread wait for the GPU. A frame the GPU is
	// too far behind to have a free set for is skipped, and its times and counts both go. Render thread only.
	//
	// Counting costs GPU time (the driver drains the pipeline at each segment boundary), so it's off until
	// SetCountStatistics turns it on, and the statistics queries are only made then.
	//
	// Statistics are counted in segments that start and end at scope boundaries, so scopes can nest though no two
	// queries of a kind can be active at once: a scope's figures are the sum of the segments inside it. Nothing is
	// counted outside every scope.
	class GPUProfiler
	{
	public:
		GPUProfiler();
		~GPUProfiler();

		GPUProfiler(const GPUProfiler&) = delete;
		GPUProfiler& operator=(const GPUProfiler&) = delete;

	public:
		// Makes the timestamps, with the device attached to the calling thread. Statistics are optional: without
		// createStatistics, frames never count them.
		void Initialize(const GPUTimestampsFactory& createTimestamps, const GPUStatisticsFactory& createStatistics);
		// Destroys the queries, with the device still attached to the calling thread
		void Release();

		// Whether frames count pipeline statistics, from the next frame begun: a frame counts them throughout or not at
		// all. The first time on, it makes every frame's statistics queries; if the device can't, it turns
		// CanCountStatistics false and frames go uncounted.
		void SetCountStatistics(bool count);
		// False without a statistics factory, or once the device has failed to make them
		bool CanCountStatistics() const { return m_canCountStatistics; }

		// Before each batch of commands: starts a frame unless one is open
		void BeginFrameIfNeeded();
		// Scopes nest. One that doesn't fit in the frame's timestamps isn't timed or counted, and doesn't split the
		// segments of the scope around it, which still counts its draws.
		void BeginScope(U32 id);
		void EndScope();
		// Just before the frame presents: ends it, then reads back the frames the GPU has finished. True with the newest
		// of them in outProfile, false if none finished. outProfile keeps its vectors' memory between calls.
		bool EndFrame(GPUFrameProfile& outProfile);

	private:
		struct Frame;

	public:
		// The timestamps a frame can write: two a scope, and the frame's first and last. Also the statistics segments it
		// can count: no more than two a scope.
		static constexpr U32 k_capacity = 256;
		// Frames in flight before one is skipped: a frame is read back up to k_frameCount - 1 frames later
		static constexpr size_t k_frameCount = 4;

	private:
		// Every frame's statistics queries, or none (false) if the device can't make them all
		bool MakeStatistics();
		// Ends the open statistics segment, if there is one, and begins the next if a scope is still open
		void SplitSegment(Frame& frame, bool beginNext);
		void ReadFinishedFrames(GPUFrameProfile& outProfile, bool& outHasProfile);

	private:
		std::array<std::unique_ptr<Frame>, k_frameCount> m_frames;
		// The frame being written, if m_isFrameOpen, or the next to be
		size_t m_current = 0;
		// The oldest frame not read back yet, if any is pending
		size_t m_oldest = 0;
		bool m_isFrameOpen = false;

		// Kept until the first frame counts, which calls it
		GPUStatisticsFactory m_createStatistics;
		bool m_canCountStatistics = false;
		// What the next frame begun does: SetCountStatistics, and whether MakeStatistics made the queries
		bool m_countStatistics = false;
		bool m_hasStatisticsQueries = false;
		// Kept to reuse their memory
		std::vector<double> m_seconds;
		std::vector<GPUPipelineStatistics> m_segments;
	};
}

#endif // RZE_RENDER_STATS
