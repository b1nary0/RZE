#pragma once

#include <string>
#include <utility>
#include <vector>

// Records every frame's timings for A/B performance comparisons, enabled from the command line:
//   -perfcapture <file>      where to write the samples, one "frameMs,updateMs" line per frame, where updateMs is
//                            RZE_Engine::PreUIUpdate() + RZE_Engine::Update()
//   -perfwarmup <seconds>    how long to wait before recording, e.g. for the scene to load (default 10)
//   -perfduration <seconds>  how long to record (default 15)
// When the duration is up the samples are written and the engine exits.
class FrameTimeCapture
{
public:
	FrameTimeCapture() = default;
	~FrameTimeCapture() = default;

public:
	// Reads the command line; the capture stays disabled without -perfcapture
	void Initialize();

	bool IsEnabled() const { return m_isEnabled; }

	// Once per frame. elapsedSeconds is the time since the engine loop started; frameMs and updateMs are the
	// previous frame's total and RZE_Engine::PreUIUpdate() + Update() times. Returns true once the capture has
	// been written, meaning the engine should exit.
	bool AddFrame(double elapsedSeconds, double frameMs, double updateMs);

private:
	void Write() const;

private:
	bool m_isEnabled = false;
	std::string m_outputPath;
	double m_warmupSeconds = 10.0;
	double m_durationSeconds = 15.0;

	// frameMs, updateMs
	std::vector<std::pair<float, float>> m_samples;
};
