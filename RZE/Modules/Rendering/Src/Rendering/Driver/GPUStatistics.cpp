#include <Rendering/Driver/GPUStatistics.h>

// Render stats only (its header is empty without them)
#ifdef RZE_RENDER_STATS

namespace Rendering
{
	GPUPipelineStatistics& GPUPipelineStatistics::operator+=(const GPUPipelineStatistics& other)
	{
		TrianglesSubmitted += other.TrianglesSubmitted;
		VerticesShaded += other.VerticesShaded;
		TrianglesInView += other.TrianglesInView;
		PixelsShaded += other.PixelsShaded;
		return *this;
	}

	GPUPipelineStatistics GPUPipelineStatistics::operator*(double scale) const
	{
		GPUPipelineStatistics scaled;
		scaled.TrianglesSubmitted = TrianglesSubmitted * scale;
		scaled.VerticesShaded = VerticesShaded * scale;
		scaled.TrianglesInView = TrianglesInView * scale;
		scaled.PixelsShaded = PixelsShaded * scale;
		return scaled;
	}
}

#endif // RZE_RENDER_STATS
