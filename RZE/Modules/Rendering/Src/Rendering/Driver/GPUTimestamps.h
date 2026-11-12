#pragma once

#include <Rendering/Driver/GPUQueryReadResult.h>

#include <Utils/PrimitiveDefs.h>

#include <vector>

namespace Rendering
{
	// One frame's GPU timestamps (GPUProfiler): the GPU writes the time when it reaches each Write in the command stream.
	// Each graphics API has its own (Driver/<API>/), made by IGFXDevice::CreateGPUTimestamps. Render thread only.
	class IGPUTimestamps
	{
	public:
		IGPUTimestamps() = default;
		virtual ~IGPUTimestamps() = default;

		IGPUTimestamps(const IGPUTimestamps&) = delete;
		IGPUTimestamps& operator=(const IGPUTimestamps&) = delete;

	public:
		// Starts a frame of timestamps; what the set held before is no longer read
		virtual void Begin() = 0;
		// The GPU writes timestamp index (below the capacity the set was made with) when it reaches this point
		virtual void Write(U32 index) = 0;
		// Ends the frame: no Write until the next Begin
		virtual void End() = 0;
		// Once the GPU has passed every Write of the frame (Ready): the first count timestamps, as seconds after the
		// first. Never waits for the GPU.
		virtual EGPUQueryReadResult Read(U32 count, std::vector<double>& outSeconds) = 0;
	};
}
