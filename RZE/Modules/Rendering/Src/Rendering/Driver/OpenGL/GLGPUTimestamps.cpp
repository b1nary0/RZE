#include <Rendering/Driver/OpenGL/GLGPUTimestamps.h>

#include <Rendering/Driver/OpenGL/GLCommon.h>

namespace Rendering
{
	GLGPUTimestamps::GLGPUTimestamps(U32 capacity)
		: m_queries(capacity, 0)
	{
		glCreateQueries(GL_TIMESTAMP, static_cast<GLsizei>(capacity), m_queries.data());
	}

	GLGPUTimestamps::~GLGPUTimestamps()
	{
		glDeleteQueries(static_cast<GLsizei>(m_queries.size()), m_queries.data());
	}

	void GLGPUTimestamps::Begin()
	{
	}

	void GLGPUTimestamps::Write(U32 index)
	{
		AssertExpr(index < m_queries.size());
		glQueryCounter(m_queries[index], GL_TIMESTAMP);
	}

	void GLGPUTimestamps::End()
	{
	}

	EGPUQueryReadResult GLGPUTimestamps::Read(U32 count, std::vector<double>& outSeconds)
	{
		AssertExpr(count > 0 && count <= m_queries.size());

		// The GPU writes them in order, so once the last is there, so are the others
		GLint isAvailable = GL_FALSE;
		glGetQueryObjectiv(m_queries[count - 1], GL_QUERY_RESULT_AVAILABLE, &isAvailable);
		if (isAvailable == GL_FALSE)
		{
			return EGPUQueryReadResult::NotReady;
		}

		outSeconds.resize(count);
		GLuint64 first = 0;
		for (U32 index = 0; index < count; ++index)
		{
			GLuint64 nanoseconds = 0;
			glGetQueryObjectui64v(m_queries[index], GL_QUERY_RESULT, &nanoseconds);
			if (index == 0)
			{
				first = nanoseconds;
			}
			outSeconds[index] = static_cast<double>(nanoseconds - first) * 1e-9;
		}
		return EGPUQueryReadResult::Ready;
	}
}
