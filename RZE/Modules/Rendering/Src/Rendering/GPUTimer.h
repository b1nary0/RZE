#pragma once

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

	// One frame's times on the GPU (GPUTimer)
	struct GPUFrameTimes
	{
		// From the frame's first command to its last before presenting
		double FrameMS = 0.0;
		// Indexed by scope ID (Renderer::BeginGPUScope), each scope with that ID summed over the frame; 0 for an ID the
		// frame didn't time
		std::vector<double> ScopeMS;
	};

	// Times frames, and scopes within them, on the GPU, through the device's timestamps, the same way on every graphics
	// API. A frame's times are read back a few frames later, once the GPU has finished it, so timing never makes the
	// render thread wait for the GPU. A frame the GPU is too far behind to have a free set for isn't timed. Render
	// thread only.
	class GPUTimer
	{
	public:
		GPUTimer();
		~GPUTimer();

		GPUTimer(const GPUTimer&) = delete;
		GPUTimer& operator=(const GPUTimer&) = delete;

	public:
		// Makes the timestamps, with the device attached to the calling thread
		void Initialize(const GPUTimestampsFactory& createTimestamps);
		// Destroys them, with the device still attached to the calling thread
		void Release();

		// Before each batch of commands: starts a frame unless one is open
		void BeginFrameIfNeeded();
		// Scopes nest. One that doesn't fit in the frame's timestamps isn't timed.
		void BeginScope(U32 id);
		void EndScope();
		// Just before the frame presents: ends it, then reads back the frames the GPU has finished. True with the newest
		// of them in outTimes, false if none finished.
		bool EndFrame(GPUFrameTimes& outTimes);

	private:
		struct Frame;

	public:
		// The timestamps a frame can write: two a scope, and the frame's first and last
		static constexpr U32 k_capacity = 256;
		// Frames in flight before one goes untimed: a frame is read back up to k_frameCount - 1 frames later
		static constexpr size_t k_frameCount = 4;

	private:
		void ReadFinishedFrames(GPUFrameTimes& outTimes, bool& outHasTimes);

	private:
		std::array<std::unique_ptr<Frame>, k_frameCount> m_frames;
		// The frame being written, if m_isFrameOpen, or the next to be
		size_t m_current = 0;
		// The oldest frame not read back yet, if any is pending
		size_t m_oldest = 0;
		bool m_isFrameOpen = false;
		// Kept to reuse its memory
		std::vector<double> m_seconds;
	};
}
