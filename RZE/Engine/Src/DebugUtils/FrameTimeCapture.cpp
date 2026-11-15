#include <DebugUtils/FrameTimeCapture.h>

// Render stats only (its header is empty without them)
#ifdef RZE_RENDER_STATS

#include <Graphics/RenderEngine.h>

#include <Utils/Platform/CmdLine.h>

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iterator>

namespace
{
	// GPUPipelineStatistics' columns, in WriteStatistics' order
	constexpr const char* k_statisticsHeader = "triangles,verticesShaded,trianglesInView,pixelsShaded";

	// Rendering::EStateCategory's columns, in its order: each is written as <name>Binds then <name>Repeats
	constexpr const char* k_stateColumns[] = { "targets", "shaders", "fixedState", "constantBuffers", "textures", "vertexBuffers", "indexBuffers" };
	static_assert(std::size(k_stateColumns) == Rendering::k_stateCategoryCount, "a column for each state category");

	void WriteStatistics(std::ofstream& file, const Rendering::GPUPipelineStatistics& statistics)
	{
		// Whole numbers, however large: the stream's default precision would write 1.2e+06
		file << std::fixed << std::setprecision(0)
			<< statistics.TrianglesSubmitted << ',' << statistics.VerticesShaded << ',' << statistics.TrianglesInView << ','
			<< statistics.PixelsShaded << std::defaultfloat << std::setprecision(6);
	}
}

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

bool FrameTimeCapture::AddFrame(double elapsedSeconds, const FrameTimeSample& sample, const RenderStats& renderStats)
{
	if (!m_isEnabled || elapsedSeconds < m_warmupSeconds)
	{
		return false;
	}

	if (elapsedSeconds < m_warmupSeconds + m_durationSeconds)
	{
		m_samples.push_back(sample);

		// The stages don't change while a scene renders; if they do, the totals start over with the new ones
		const bool isSameStages = m_stageTotals.size() == renderStats.Stages.size()
			&& std::equal(m_stageTotals.begin(), m_stageTotals.end(), renderStats.Stages.begin(), [](const StageTotals& totals, const RenderStageStats& stage)
				{
					return totals.Name == stage.Name;
				});
		if (!isSameStages)
		{
			m_stageTotals.clear();
			m_stageFrameCount = 0;
			for (const RenderStageStats& stage : renderStats.Stages)
			{
				StageTotals totals;
				totals.Name = stage.Name;
				m_stageTotals.push_back(std::move(totals));
			}
		}
		for (size_t index = 0; index < m_stageTotals.size(); ++index)
		{
			m_stageTotals[index].TimeMS += renderStats.Stages[index].TimeMS;
			m_stageTotals[index].DrawCalls += renderStats.Stages[index].DrawCalls;
			m_stageTotals[index].Instances += renderStats.Stages[index].Instances;
			for (U32 category = 0; category < Rendering::k_stateCategoryCount; ++category)
			{
				m_stageTotals[index].StateChanges[category] += renderStats.Stages[index].StateChanges[category];
			}
			m_stageTotals[index].GPUTimeMS += renderStats.Stages[index].GPUTimeMS;
			m_stageTotals[index].GPUStatistics += renderStats.Stages[index].GPUStatistics;
		}
		++m_stageFrameCount;
		return false;
	}

	WriteFramesCSV();
	WriteStagesCSV();
	m_isEnabled = false;
	return true;
}

void FrameTimeCapture::WriteFramesCSV() const
{
	std::ofstream file(m_outputPath);
	if (!file)
	{
		RZE_LOG_ERROR_ARGS("Frame time capture: couldn't open %s for writing", m_outputPath.c_str());
		return;
	}

	file << "frameMs,updateMs,recordMs,renderBusyMs,renderPresentMs,gpuMs,drawCalls,instances," << k_statisticsHeader << '\n';
	for (const FrameTimeSample& sample : m_samples)
	{
		file << sample.FrameMS << ',' << sample.UpdateMS << ',' << sample.RecordMS << ',' << sample.RenderThreadBusyMS << ','
			<< sample.RenderThreadPresentMS << ',' << sample.GPUMS << ',' << sample.DrawCalls << ',' << sample.Instances << ',';
		WriteStatistics(file, sample.GPUStatistics);
		file << '\n';
	}
}

void FrameTimeCapture::WriteStagesCSV() const
{
	const std::string stagesPath = m_outputPath + ".stages.csv";
	std::ofstream file(stagesPath);
	if (!file)
	{
		RZE_LOG_ERROR_ARGS("Frame time capture: couldn't open %s for writing", stagesPath.c_str());
		return;
	}

	// Averages of averages: each frame added RenderStats' figures, which are per frame over its stats window
	const double frameCount = static_cast<double>(std::max<size_t>(m_stageFrameCount, 1));
	file << "stage,timeMs,drawCalls,gpuMs,instances," << k_statisticsHeader;
	for (U32 category = 0; category < Rendering::k_stateCategoryCount; ++category)
	{
		file << ',' << k_stateColumns[category] << "Binds," << k_stateColumns[category] << "Repeats";
	}
	file << '\n';
	for (const StageTotals& totals : m_stageTotals)
	{
		file << totals.Name << ',' << totals.TimeMS / frameCount << ',' << totals.DrawCalls / frameCount << ',' << totals.GPUTimeMS / frameCount << ','
			<< totals.Instances / frameCount << ',';
		WriteStatistics(file, totals.GPUStatistics * (1.0 / frameCount));
		for (U32 category = 0; category < Rendering::k_stateCategoryCount; ++category)
		{
			file << ',' << totals.StateChanges[category].Binds / frameCount << ',' << totals.StateChanges[category].Repeats / frameCount;
		}
		file << '\n';
	}
}

#endif // RZE_RENDER_STATS
