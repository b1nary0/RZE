#include <StdAfx.h>
#include <DebugUtils/FrameTimeCapture.h>

#include <Utils/Platform/CmdLine.h>

#include <fstream>

void FrameTimeCapture::Initialize()
{
	std::string_view outputPath;
	if (!CmdLine::Arguments::Get("-perfcapture", outputPath) || outputPath.empty())
	{
		return;
	}

	int warmupSeconds = static_cast<int>(m_warmupSeconds);
	int durationSeconds = static_cast<int>(m_durationSeconds);
	CmdLine::Arguments::GetInt("-perfwarmup", warmupSeconds);
	CmdLine::Arguments::GetInt("-perfduration", durationSeconds);

	m_isEnabled = true;
	m_outputPath = std::string(outputPath);
	m_warmupSeconds = warmupSeconds;
	m_durationSeconds = durationSeconds;

	// Enough for the duration at a few thousand frames a second, so recording doesn't reallocate
	m_samples.reserve(static_cast<size_t>(durationSeconds) * 4000);

	RZE_LOG_ARGS("Frame time capture: recording %d s after a %d s warm-up, to %s", durationSeconds, warmupSeconds, m_outputPath.c_str());
}

bool FrameTimeCapture::AddFrame(double elapsedSeconds, double frameMs, double updateMs)
{
	if (!m_isEnabled || elapsedSeconds < m_warmupSeconds)
	{
		return false;
	}

	if (elapsedSeconds < m_warmupSeconds + m_durationSeconds)
	{
		m_samples.emplace_back(static_cast<float>(frameMs), static_cast<float>(updateMs));
		return false;
	}

	Write();
	m_isEnabled = false;
	return true;
}

void FrameTimeCapture::Write() const
{
	std::ofstream file(m_outputPath);
	if (!file)
	{
		RZE_LOG_ERROR_ARGS("Frame time capture: couldn't open %s for writing", m_outputPath.c_str());
		return;
	}

	for (const std::pair<float, float>& sample : m_samples)
	{
		file << sample.first << ',' << sample.second << '\n';
	}
}
