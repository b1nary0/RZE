#pragma once

#include <Rendering/Driver/GPUTimestamps.h>

namespace Rendering
{
	// GL_TIMESTAMP queries, one per timestamp. GL's timestamps are nanoseconds on one clock, so a frame needs no query
	// of its own around them.
	class GLGPUTimestamps : public IGPUTimestamps
	{
	public:
		explicit GLGPUTimestamps(U32 capacity);
		~GLGPUTimestamps() override;

	// IGPUTimestamps interface
	public:
		void Begin() override;
		void Write(U32 index) override;
		void End() override;
		EGPUQueryReadResult Read(U32 count, std::vector<double>& outSeconds) override;

	private:
		std::vector<U32> m_queries;
	};
}
