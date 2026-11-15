#pragma once

#include <Utils/DebugUtils/DebugConfig.h>

// Render stats only
#ifdef RZE_RENDER_STATS

#include <Rendering/Driver/GPUQueryReadResult.h>

#include <Utils/PrimitiveDefs.h>

#include <vector>

namespace Rendering
{
	// What the GPU counted while it ran some commands: the graphics API's pipeline statistics, the same four on every API.
	// Counts as the GPU reports them, so the details vary by driver (how many vertices it reuses rather than shades
	// again, how it clips): compare them on the same machine and API. Doubles, so averages fit the same struct; they're
	// exact up to 2^53.
	struct GPUPipelineStatistics
	{
		// Primitives the draws submitted: their triangles (lines and points too, where a draw has them)
		double TrianglesSubmitted = 0.0;
		// Vertex shader invocations: below 3 a triangle when the GPU reuses a vertex it shaded for a neighbour
		double VerticesShaded = 0.0;
		// Primitives left after clipping to the view: those wholly outside it are gone, and a clipped one can become
		// several. Back faces are culled after this, so they're still counted.
		double TrianglesInView = 0.0;
		// Pixel shader invocations: none for a draw with no pixel shader, on most drivers
		double PixelsShaded = 0.0;

		GPUPipelineStatistics& operator+=(const GPUPipelineStatistics& other);
		// Each count scaled, e.g. a sum over a window of frames to a frame's average
		GPUPipelineStatistics operator*(double scale) const;
	};

	// One frame's pipeline statistics (GPUProfiler), in segments: the GPU counts what it runs between a BeginSegment and
	// its EndSegment. Segments don't overlap, so no two queries of a kind are ever active together (OpenGL allows only
	// one at a time). Each graphics API has its own (Driver/<API>/), made by IGFXDevice::CreateGPUStatistics. Render
	// thread only.
	class IGPUStatistics
	{
	public:
		IGPUStatistics() = default;
		virtual ~IGPUStatistics() = default;

		IGPUStatistics(const IGPUStatistics&) = delete;
		IGPUStatistics& operator=(const IGPUStatistics&) = delete;

	public:
		// Starts a frame of segments; what the set held before is no longer read
		virtual void Begin() = 0;
		// The GPU starts counting segment index (below the capacity the set was made with). No segment may be open.
		virtual void BeginSegment(U32 index) = 0;
		// The GPU stops counting the open segment
		virtual void EndSegment() = 0;
		// Ends the frame: no segment is open, and none begins until the next Begin
		virtual void End() = 0;
		// Once the GPU has finished every segment of the frame (Ready): the first count segments' statistics. Never waits
		// for the GPU. Ready with none when count is 0.
		virtual EGPUQueryReadResult Read(U32 count, std::vector<GPUPipelineStatistics>& outSegments) = 0;
	};
}

#endif // RZE_RENDER_STATS
