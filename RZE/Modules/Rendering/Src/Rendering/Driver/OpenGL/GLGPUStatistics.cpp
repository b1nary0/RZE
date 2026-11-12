#include <Rendering/Driver/OpenGL/GLGPUStatistics.h>

#include <Rendering/Driver/OpenGL/GLCommon.h>

#include <array>

namespace Rendering
{
	namespace
	{
		// One query each, in GPUPipelineStatistics' order
		constexpr std::array<GLenum, 4> k_targets = {
			GL_PRIMITIVES_SUBMITTED_ARB,
			GL_VERTEX_SHADER_INVOCATIONS_ARB,
			GL_CLIPPING_OUTPUT_PRIMITIVES_ARB,
			GL_FRAGMENT_SHADER_INVOCATIONS_ARB,
		};
		constexpr U32 k_statisticCount = static_cast<U32>(k_targets.size());
	}

	GLGPUStatistics::GLGPUStatistics(U32 capacity)
		: m_queries(static_cast<size_t>(capacity) * k_statisticCount, 0)
	{
		// Names only: each query object is made by its first glBeginQuery, with its target. glCreateQueries would need the
		// target per name now, and it's one call per target either way.
		glGenQueries(static_cast<GLsizei>(m_queries.size()), m_queries.data());
	}

	GLGPUStatistics::~GLGPUStatistics()
	{
		glDeleteQueries(static_cast<GLsizei>(m_queries.size()), m_queries.data());
	}

	void GLGPUStatistics::Begin()
	{
		AssertExpr(!m_isSegmentOpen);
	}

	void GLGPUStatistics::BeginSegment(U32 index)
	{
		AssertExpr(!m_isSegmentOpen);
		AssertExpr((static_cast<size_t>(index) + 1) * k_statisticCount <= m_queries.size());

		const size_t first = static_cast<size_t>(index) * k_statisticCount;
		for (U32 statistic = 0; statistic < k_statisticCount; ++statistic)
		{
			glBeginQuery(k_targets[statistic], m_queries[first + statistic]);
		}
		m_isSegmentOpen = true;
	}

	void GLGPUStatistics::EndSegment()
	{
		AssertExpr(m_isSegmentOpen);

		for (GLenum target : k_targets)
		{
			glEndQuery(target);
		}
		m_isSegmentOpen = false;
	}

	void GLGPUStatistics::End()
	{
		AssertExpr(!m_isSegmentOpen);
	}

	bool GLGPUStatistics::IsSegmentAvailable(U32 index) const
	{
		const size_t first = static_cast<size_t>(index) * k_statisticCount;
		for (U32 statistic = 0; statistic < k_statisticCount; ++statistic)
		{
			GLint isAvailable = GL_FALSE;
			glGetQueryObjectiv(m_queries[first + statistic], GL_QUERY_RESULT_AVAILABLE, &isAvailable);
			if (isAvailable == GL_FALSE)
			{
				return false;
			}
		}
		return true;
	}

	EGPUQueryReadResult GLGPUStatistics::Read(U32 count, std::vector<GPUPipelineStatistics>& outSegments)
	{
		AssertExpr(static_cast<size_t>(count) * k_statisticCount <= m_queries.size());

		outSegments.resize(count);
		if (count == 0)
		{
			return EGPUQueryReadResult::Ready;
		}

		// The last segment first, the usual one to still be running, then every other: GL doesn't promise that queries of
		// different kinds finish in order, so none is assumed to be done because a later one is
		if (!IsSegmentAvailable(count - 1))
		{
			return EGPUQueryReadResult::NotReady;
		}
		for (U32 segment = 0; segment + 1 < count; ++segment)
		{
			if (!IsSegmentAvailable(segment))
			{
				return EGPUQueryReadResult::NotReady;
			}
		}

		// Every result is there, so GL_QUERY_RESULT returns at once
		for (U32 segment = 0; segment < count; ++segment)
		{
			const size_t first = static_cast<size_t>(segment) * k_statisticCount;
			std::array<GLuint64, k_statisticCount> values = {};
			for (U32 statistic = 0; statistic < k_statisticCount; ++statistic)
			{
				glGetQueryObjectui64v(m_queries[first + statistic], GL_QUERY_RESULT, &values[statistic]);
			}

			GPUPipelineStatistics& statistics = outSegments[segment];
			statistics.TrianglesSubmitted = static_cast<double>(values[0]);
			statistics.VerticesShaded = static_cast<double>(values[1]);
			statistics.TrianglesInView = static_cast<double>(values[2]);
			statistics.PixelsShaded = static_cast<double>(values[3]);
		}
		return EGPUQueryReadResult::Ready;
	}
}
