#pragma once

#include <Utils/DebugUtils/DebugConfig.h>

// Render stats only
#ifdef RZE_RENDER_STATS

#include <Rendering/Driver/GPUStatistics.h>

namespace Rendering
{
	// ARB_pipeline_statistics_query (core in GL 4.6): one query a statistic, so four a segment, begun and ended
	// together. GL allows one active query of each kind; segments never overlap, so one of each is.
	class GLGPUStatistics : public IGPUStatistics
	{
	public:
		explicit GLGPUStatistics(U32 capacity);
		~GLGPUStatistics() override;

	// IGPUStatistics interface
	public:
		void Begin() override;
		void BeginSegment(U32 index) override;
		void EndSegment() override;
		void End() override;
		EGPUQueryReadResult Read(U32 count, std::vector<GPUPipelineStatistics>& outSegments) override;

	private:
		// Whether every query of segment index has its result, without waiting for any
		bool IsSegmentAvailable(U32 index) const;

	private:
		// k_statisticCount a segment, in the order of GLGPUStatistics.cpp's targets
		std::vector<U32> m_queries;
		bool m_isSegmentOpen = false;
	};
}

#endif // RZE_RENDER_STATS
